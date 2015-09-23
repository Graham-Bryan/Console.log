
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};


function locationSuccess(pos) {
  
  var dictionary = {
        "KEY_LATITUDE":  ""+pos.coords.latitude,
        "KEY_LONGITUDE": ""+pos.coords.longitude
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Location info sent to Pebble successfully! ");
        },
        function(e) {
          console.log("Error sending location info to Pebble! ");
        }
      );
}
 
function requestWeatherData(pos) {
  // Construct URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude;  
  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature is " + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully! ");
        },
        function(e) {
          console.log("Error sending weather info to Pebble! ");
        }
      );
    }      
  );
}

function requestDaylightData(pos) {
  // Construct URL
  var url = "http://api.sunrise-sunset.org/json?lat=" +
      pos.coords.latitude + "&lng=" + pos.coords.longitude;  
  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      console.log("Sunrise is " + json.results.sunrise);
      console.log("Sunset are " + json.results.sunset);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_SUNRISE": json.results.sunrise,
        "KEY_SUNSET": json.results.sunset
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Daylight info sent to Pebble successfully! ");
        },
        function(e) {
          console.log("Error sending Daylight info to Pebble! ");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getLocation(successFunc) {
  
  navigator.geolocation.getCurrentPosition(
    successFunc,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}


  Pebble.addEventListener('appmessage',
    function(e) {
      console.log("AppMessage received! " + e.payload.cmd);
      
      if (e.payload.cmd == "daylight")
        getLocation(requestDaylightData);
      if (e.payload.cmd == "weather")
        getLocation(requestWeatherData);
       if (e.payload.cmd == "location")
        getLocation(locationSuccess);     
    }                     
  );

