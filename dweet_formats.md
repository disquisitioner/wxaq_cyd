## Special Note: Dweet Data Formats
Dweet.io allows any subscriber to fetch the most recent message ("dweet") published by any device, and supports a wide range of programming languages as well an HTML-based web API. (More details are on the dweet.io main page.) Extracting information from those messages means understanding the format of what the device publishes, along with the wrapper Dweet puts around that information in returing it as a single JSON-formatted payload to the subscriber.

The [ArduinoJson](https://arduinojson.org/) library used in `wxaq_cyd` is a very powerful tool for extracting values from within even the most complex JSON, and it is very well documented. If you're interested in understanding how that JSON-manipulating code in `wxaq_cyd` works you need to know the published data format my devices use so I've documented that here.

### Home Weather Station
The data schema here is intended to make everything about the environmental data reported by my home weather station self explanatory, which means each value is accompanied by a string containing its name and another identifying the measurement units used for it.
```
{
  "this": "succeeded",
  "by": "getting",
  "the": "dweets",
  "with": [
    {
      "thing": "myhome-wxstation",
      "created": "2024-11-25T23:20:21.799Z",
      "content": {
        "sensors": [
          {
            "units": "degrees F",
            "name": "Outdoor Temperature",
            "value": "52.64"
          },
          {
            "units": "degrees F",
            "name": "Indoor Temperature",
            "value": "66.94"
          },
          {
            "units": "inches Hg",
            "name": "Barometric pressure",
            "value": "28.58"
          },
          {
            "units": "MPH",
            "name": "Wind Speed",
            "value": "6.17"
          },
          {
            "units": "MPH",
            "name": "Wind Gust",
            "value": "14.33"
          },
          {
            "units": "inches",
            "name": "Rainfall",
            "value": "0.00"
          },
          {
            "units": "inches",
            "name": "Rainfall Today",
            "value": "0.46"
          }
        ],
        "time": "2024-11-25T15:20:21-0800"
      }
    }
  ]
}
```
### Air Quality Monitor
The data schema for my air quality monitor is simpler than that for the weather station, and in addition to some environmental readings includes data about the device itself. I've found it helpful for devices to include hardware and system status information in their dweets to help with troubleshooting, monitoring battery status, etc.
```
{
  "this": "succeeded",
  "by": "getting",
  "the": "dweets",
  "with": [
    {
      "thing": "myhome-aqimonitor",
      "created": "2024-11-25T23:31:38.728Z",
      "content": {
        "wifi_rssi": -53,
        "AQI": 7.46,
        "address": "192.168.254.54",
        "temperature": 54.8,
        "vocIndex": 3,
        "humidity": 88.9,
        "PM25_value": 1.79,
        "min_AQI": 0.91,
        "max_AQI": 505
      }
    }
  ]
}
```
