#include <pebble.h>
  
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_LATITUDE 2
#define KEY_LONGITUDE 3
#define KEY_SUNRISE 4
#define KEY_SUNSET 5  
  
#define TEXT_COLOR GColorWhite
#define BG_COLOR GColorBlack
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static ScrollLayer *s_scroll_layer;
static TextLayer *s_console_layer;
static BitmapLayer *s_topbar_layer;
static BitmapLayer *s_bottombar_layer;

static GFont title_font;
static GFont content_font;

static char log_buffer[1000];
static uint char_count;

static AppTimer *timer;

static GRect top_frame;
static GRect bottom_frame;
static GRect scroll_frame;

uint data_step = 0;

static void update_time() {
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char time_buffer[12];
  static char date_buffer[12];  

  if(clock_is_24h_style() == true) {
    strftime(time_buffer, 12, "%H:%M %Z", tick_time); 
  }else{
    strftime(time_buffer, 12, "%I:%M%p %Z", tick_time);     
  }

  
  
  strftime(date_buffer, 12, "LOG%y.%m.%d", tick_time); 

  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, date_buffer);  
}

static void update_content()
{
  
  static char display_buffer[1000];
  
  strncpy (display_buffer, log_buffer, char_count);
  
  text_layer_set_text(s_console_layer, display_buffer);
  
  GSize content_size = graphics_text_layout_get_content_size(text_layer_get_text(s_console_layer), content_font, GRect(0,0,scroll_frame.size.w,2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  
  text_layer_set_size(s_console_layer, content_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(scroll_frame.size.w, content_size.h));
  
  int offset = -content_size.h;
  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0,offset * 18), false);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Content Size: %d", content_size.h);     
}

static void step_content()
{
  char_count++;
  uint log_length = strlen(log_buffer);
  
  if (char_count < log_length)
  {
    char c = log_buffer[char_count-1];
    
    //if (c == '.')
    //  timer = app_timer_register(500, (AppTimerCallback) step_content, NULL);
    if (c == '\n')
      timer = app_timer_register(1000, (AppTimerCallback) step_content, NULL);
    else 
      timer = app_timer_register(100, (AppTimerCallback) step_content, NULL);
  }else{
    if (timer != NULL)
      app_timer_cancel (timer);
    timer = NULL;
  }
  
  update_content();
}

static void add_content(char *content, bool animated)
{
  strcat(log_buffer, content);
  strcat(log_buffer, "\n");  
  if (!animated){
    char_count += strlen(content);
    update_content();
  }else if (timer == NULL){
    step_content();
  }
}

static void request_weather()
{
    add_content("EXT CONDITIONS...", true);  
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter,99,  "weather");
    app_message_outbox_send();  
}

static void request_location()
{
    add_content("LOCATING...", true);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter,99,  "location");
    app_message_outbox_send();  
}

static void request_daylight()
{
    add_content("DAYLIGHT...", true);  
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter,99,  "daylight");
    app_message_outbox_send();  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  switch(data_step)
  {
    case 0:{
        request_location();  
    }break;
    case 1:{
        request_weather();  
    }break;
    case 2:{
        request_daylight();  
    }break;    
    default:break;
  }
  data_step++;
  
  if (data_step > 2)
    data_step = 0;
}

static TextLayer* create_log_text_layer(GRect frame, GFont font, GColor color)
{
  TextLayer *layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, color);

  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentLeft);
  
  return layer;
}

static void main_window_load(Window *window) {
  
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  title_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_20));
  content_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_14));  
  
  int m = 2;
  
  top_frame = GRect(m, m, bounds.size.w - (m*2), 30);
  bottom_frame = GRect(m, bounds.size.h - 30, bounds.size.w - (m*2), 30);
  scroll_frame = GRect(m, 33, bounds.size.w - (m*2), bounds.size.h - (66));
  
  s_time_layer = create_log_text_layer(bottom_frame,title_font, TEXT_COLOR);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  s_topbar_layer = bitmap_layer_create(GRect(0, 29, bounds.size.w, 1));
  bitmap_layer_set_background_color(s_topbar_layer ,TEXT_COLOR);
  layer_add_child(window_get_root_layer(window), (Layer*)s_topbar_layer);
  
  s_console_layer = create_log_text_layer(GRectZero,content_font, TEXT_COLOR);
  text_layer_set_text(s_console_layer, "");

  s_scroll_layer = scroll_layer_create(scroll_frame);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_console_layer)); 
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);
  layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(s_scroll_layer));  
  
  s_bottombar_layer = bitmap_layer_create(GRect(0, bottom_frame.origin.y, bounds.size.w, 1));
  bitmap_layer_set_background_color(s_bottombar_layer, TEXT_COLOR);
  layer_add_child(window_get_root_layer(window), (Layer*)s_bottombar_layer);
    
  s_date_layer = create_log_text_layer(top_frame,title_font, TEXT_COLOR);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer)); 
  
   
  
  add_content("INITIALISE...", true);
  add_content("ACTIVATED", true);  

//   s_weather_layer = create_log_text_layer(2,content_font, GColorIslamicGreen   );
//   text_layer_set_text(s_weather_layer, "Conditions");
//   layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

//   s_connection_layer = create_log_text_layer(3,content_font, GColorIslamicGreen   );
//   text_layer_set_text(s_connection_layer, "Connection");
//   layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_connection_layer));
  
//   s_battery_layer = create_log_text_layer(4,content_font, GColorIslamicGreen   );
//   text_layer_set_text(s_battery_layer, "Battery Level");
//   layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
//   s_force_layer = create_log_text_layer(5,content_font, GColorIslamicGreen   );
//   text_layer_set_text(s_force_layer, "Force");
//   layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_force_layer));  
}

static void main_window_unload(Window *window) {

}

static void battery_handler(BatteryChargeState charge_state) {
  if (charge_state.is_charging) {
    char s[20];
    snprintf(s, 20, "CHARGING... %d", charge_state.charge_percent);
    add_content(s, true);   
    APP_LOG(APP_LOG_LEVEL_INFO, "Updated Battery Status: Charging");
  } else {
    char s[20];
    snprintf(s, 20, "ENERGY LVL %d", charge_state.charge_percent);
    add_content(s, true);    
    APP_LOG(APP_LOG_LEVEL_INFO, "Updated Battery Status: %d", charge_state.charge_percent );       
  }

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  if (t == NULL)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "NO DATA");

  // For all items
  while(t != NULL) {
    // Which key was received?

    switch(t->key) {
      case KEY_TEMPERATURE:{
        char s[12];
        snprintf(s, 12, "TMP-%dC", (int)t->value->int32);
        add_content(s, true);    
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE %dC", (int)t->value->int32);
    }break;
      case KEY_CONDITIONS:{
        char s[18];
        snprintf(s, 18, "STS-%s", t->value->cstring);
        add_content(s, true);          
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_CONDITIONS %s", t->value->cstring);
    }break;
      case KEY_LATITUDE:{
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_LATITUDE %s", t->value->cstring);
        char s[12];
        snprintf(s, 12, "LAT-%s", t->value->cstring);
        add_content(s, true);    
    }break;
      case KEY_LONGITUDE:{
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_LONGITUDE %s", t->value->cstring);
        char s[12];
        snprintf(s, 12, "LNG%s", t->value->cstring);
        add_content(s, true);          
    }break;
    case KEY_SUNRISE:{
        char s[24];
        snprintf(s, 15, "RISE-%s", t->value->cstring);
        add_content(s, true);  
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_SUNRISE %s", t->value->cstring);
    }break;
      case KEY_SUNSET:{
        char s[24];
        snprintf(s, 15, "SET -%s", t->value->cstring);
        add_content(s, true);         
        APP_LOG(APP_LOG_LEVEL_INFO, "KEY_SUNSET %s", t->value->cstring);
    }break;      
    default:
        add_content("COMMS ERROR", true);   
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
 
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void bluetooth_callback(bool connected) {
  add_content("DETECT COMMS...", true);
  if(!connected) {
    add_content("COMMS FAILED", true);    
    APP_LOG(APP_LOG_LEVEL_INFO, "Bluetooth Not Connected");
  }else{
    add_content("COMMS LINKED", true);    
    APP_LOG(APP_LOG_LEVEL_INFO, "Bluetooth Connected");
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis negative.");
    }
    break;
  case ACCEL_AXIS_Y:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Y axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "Y axis negative.");
    }
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis negative.");
    }
    break;
  }
  

}

static void init() {
  char_count = 0;
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  window_set_background_color(s_main_window, BG_COLOR);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);  
  bluetooth_connection_service_subscribe(bluetooth_callback); 
  accel_tap_service_subscribe(tap_handler);  
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  bluetooth_callback(bluetooth_connection_service_peek());
  battery_handler(battery_state_service_peek());

  update_time();

}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}