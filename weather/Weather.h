/*
   Copyright (C) 2021 SFini

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
    @file Weather.h

    Class for reading all the weather data from openweathermap.
*/
#pragma once
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "Utils.h"
#include "Config.h"
#include "Time.h"
#include "RTClib.h"
#include <mini_gzip.h>
#include <miniz.h>

#define MAX_HOURLY 24
#define MAX_FORECAST 8
#define MIN_RAIN 10
#define BUFFER_SIZE 8192
#define API_NOW_URI "/v7/weather/now"
#define API_7D_URI "/v7/weather/7d"
#define API_24H_URI "/v7/weather/24h"
#define API_MOON_URI "/v7/astronomy/moon"

/**
    Class for reading all the weather data from openweathermap.
*/
class Weather
{
public:
  DateTime currentTime; //!< Current timestamp
  String currentIcon;
  String currentText;
  int currentTemp;
  float currentFeelsLike;
  float currentPrecip;
  int currentAQI;
  int currentHumidity;

  int winddir;   //!< Wind direction
  int windspeed; //!< Wind speed
  int windscale;
  String windDirStr;


  DateTime lastUpdateTime;

  DateTime sunrise; //!< Sunrise timestamp
  DateTime sunset;  //!< Sunset timestamp
  DateTime moonrise;
  DateTime moonset;

  String moonPhaseStr;
  float moonPhase;

  DateTime hourlyTime[MAX_HOURLY]; //!< timestamp of the hourly forecast
  float hourlyTemp[MAX_HOURLY];    //!< max temperature forecast
  String hourlyMain[MAX_HOURLY];   //!< description of the hourly forecast
  String hourlyIcon[MAX_HOURLY];   //!< openweathermap icon of the forecast weather
  String hourlyText[MAX_HOURLY];

  float maxRain = 0; //!< maximum rain in mm of the day forecast
  float maxTemp = 0;
  float minTemp = 0;
  float maxPressure = 1000;
  float minPressure = 1000;
  float forecastMaxTemp[MAX_FORECAST];  //!< max temperature
  float forecastMinTemp[MAX_FORECAST];  //!< min temperature
  float forecastRain[MAX_FORECAST];     //!< max rain in mm
  float forecastHumidity[MAX_FORECAST]; //!< humidity of the dayly forecast
  float forecastPressure[MAX_FORECAST]; //!< air pressure
  String forecastText[MAX_FORECAST];
  String forecastDate[MAX_FORECAST];
  String dayOfWeek[7] = {"（日）", "（一）", "（二)", "(三)", "(四)", "(五)", "(六)"};

protected:
  uint8_t conv_str_2d(const char *p)
  {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
      v = *p - '0';
    return 10 * v + *++p - '0';
  }

  DateTime DateTimeConvert(const char *date_str, const char *time_str)
  {
    //2021-09-19T10:52+08:00

    uint16_t yearOff = conv_str_2d(date_str + 2);
    uint8_t month = conv_str_2d(date_str + 5);
    uint8_t day = conv_str_2d(date_str + 8);

    uint8_t hour = conv_str_2d(time_str);
    uint8_t minute = conv_str_2d(time_str + 3);
    uint8_t second = 0;

    log_v("%02d-%02d-%02d %02d:%02d:%02d", yearOff, month, day, hour, minute, second);
    return DateTime(yearOff, month, day, hour, minute, second);
  }

  DateTime DateTimeConvert(const char *datetime_str)
  {
    //2021-09-19T10:52+08:00
    return DateTimeConvert(datetime_str, datetime_str + 11);
  }

  String GetQWeatherAPIUri(const char *path,const char *arg = "")
  {
    String uri = "";
    uri += path;
    uri += "?location=" + String(LONGITUDE, 5) + "," + String(LATITUDE, 5);
    uri += "&unit=m&lang=cn";
    uri += "&key=" + (String)QWEATHER_API_KEY;
    uri += arg;
    return uri;
  }

  bool GetJsonDoc(String uri, DynamicJsonDocument &doc)
  {
    WiFiClientSecure client;
    HTTPClient http;
    static const char ca_cert[] PROGMEM = CA_CERT;

    String url = "";
    url += "https://";
    url += QWEATHER_SRV;
    url += uri;

    client.setCACert(ca_cert);
    client.connect(QWEATHER_SRV, QWEATHER_PORT);

    log_d("URL:%s", url.c_str());
    http.begin(client, url.c_str());

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
      log_e("GetWeather failed, error: %d\n", httpCode);
      client.stop();
      http.end();
      return false;
    }
    else
    {
      int len = http.getSize();
      uint8_t *buffer = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));

      if (len > BUFFER_SIZE)
      {
        log_e("Buffer Overflow, Response size %d , Buffer size %d", len, BUFFER_SIZE);
        free(buffer);
        buffer = nullptr;
        return false;
      }

      size_t available_size = client.available();
      log_e("Available size %d.", available_size);
      if (available_size < len)
      {
        log_e("Not complate, Available size %d.", available_size);
        free(buffer);
        buffer = nullptr;
        return false;
      }

      WiFiClient *stream_ptr = http.getStreamPtr();
      mz_ulong actual_read_size = stream_ptr->readBytes(buffer, len);
      log_d("actual_read_size size %d.", actual_read_size);

      uint8_t *result = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));
      mz_ulong result_size = BUFFER_SIZE;

      // for (size_t i = 0; i < actual_read_size; i++)
      // {
      //   Serial.printf("%02x ",buffer[i]);
      //   if(i%16==0){
      //     Serial.println();
      //   }
      // }

      actual_read_size -= 10;
      int mz_error_code = mini_gz_uncompress(result, &result_size, buffer + 10, &actual_read_size);

      log_d("result_size:%d", result_size);

      if (mz_error_code)
      {
        log_e("uncompress() Failed:%s", mz_error(mz_error_code));
        free(buffer);
        buffer = nullptr;
        free(result);
        result = nullptr;
        return false;
      }

      log_d("response:%s", result);

      DeserializationError error = deserializeJson(doc, result);
      log_d("Error Code:%d", error);

      free(buffer);
      buffer = nullptr;
      free(result);
      result = nullptr;

      if (error)
      {
        log_e("deserializeJson() failed: %s", error.c_str());
        client.stop();
        http.end();
        return false;
      }
      else
      {
        client.stop();
        http.end();
        return true;
      }
    }
  }

  bool FillNow(const DynamicJsonDocument &root) //good
  {
    DynamicJsonDocument now = root["now"];

    winddir = now["wind360"].as<int>();
    windDirStr = now["windDir"].as<char *>();
    windspeed = now["windSpeed"].as<int>();
    windscale = now["windScale"].as<int>();
    currentTime = DateTimeConvert(root["updateTime"].as<char *>());
    currentText = String(now["text"].as<char *>());
    currentTemp = now["temp"].as<float>();
    currentPrecip = now["precip"].as<float>();
    currentFeelsLike = now["feelsLike"].as<float>();
    currentHumidity = now["humidity"].as<int>();
    currentIcon = now["icon"].as<char *>();
    log_d("currentTime:%s,winDir:%d,windSpeed:%d,windScale:%d",
          currentTime.format("YYYY-MM-DD hh:mm:ss").c_str(),
          winddir,
          windspeed,
          windscale);

    return true;
  }

  bool Fill24h(const DynamicJsonDocument &root) //good
  {
    DynamicJsonDocument hourly_list = root["hourly"];
    for (int i = 0; i < MAX_HOURLY; i++)
    {
      if (i < hourly_list.size())
      {
        hourlyTime[i] = DateTimeConvert(hourly_list[i]["fxTime"].as<char *>());
        hourlyTemp[i] = hourly_list[i]["temp"].as<float>();
        hourlyIcon[i] = hourly_list[i]["icon"].as<char *>();
        hourlyText[i] = String(hourly_list[i]["text"].as<char *>());
        log_d("hourlyTime[%d]:%s,Temp:%f,Text:%s",
              i,
              hourlyTime[i].format("hh:mm"),
              hourlyTemp[i],
              hourlyText[i]
              );
      }
    }
    return true;
  }

  bool Fill7d(const DynamicJsonDocument &root) //good
  {
    DynamicJsonDocument dayly_list = root["daily"];

    for (int i = 0; i < MAX_FORECAST; i++)
    {
      if (i < dayly_list.size())
      {
        forecastMaxTemp[i] = dayly_list[i]["tempMax"].as<float>();
        forecastMinTemp[i] = dayly_list[i]["tempMin"].as<float>();
        forecastRain[i] = dayly_list[i]["precip"].as<float>();
        forecastHumidity[i] = dayly_list[i]["humidity"].as<float>();
        forecastPressure[i] = dayly_list[i]["pressure"].as<float>();
        forecastDate[i] = String(dayly_list[i]["fxDate"].as<char *>()+8); //2021-09-21
        forecastText[i] = String(dayly_list[i]["textDay"].as<char *>());
        maxRain = forecastRain[i] > maxRain ? forecastRain[i] : maxRain;
        maxTemp = forecastMaxTemp[i] > maxTemp ? forecastMaxTemp[i] : maxTemp;
        minTemp = forecastMinTemp[i] < minTemp ? forecastMinTemp[i] : minTemp;
        maxPressure = forecastPressure[i] > maxPressure ? forecastPressure[i] : maxPressure;
        minPressure = forecastPressure[i] < minPressure ? forecastPressure[i] : minPressure;
        log_i("dayly_list[%d]:tempMax:%.2f,tempMin:%.2f,precip:%.2f,humidity:%.2f,pressure:%.2f",
              i,
              forecastMaxTemp[i],
              forecastMinTemp[i],
              forecastRain[i],
              forecastHumidity[i],
              forecastPressure[i]);
      }
      if (0 == i)
      {
        maxTemp = forecastMaxTemp[0];
        minTemp = forecastMinTemp[0];
        maxPressure = forecastPressure[0];
        minPressure = forecastPressure[0];
      }
    }
    log_i("maxTemp:%.2f,minTemp:%.2f,maxRain:%.2f,maxPressure:%.2f,minPressure:%.2f",
          maxTemp,
          minTemp,
          maxRain,
          maxPressure,
          minPressure);
    sunrise = DateTimeConvert(dayly_list[0]["fxDate"].as<char *>(), dayly_list[0]["sunrise"].as<char *>());
    sunset = DateTimeConvert(dayly_list[0]["fxDate"].as<char *>(), dayly_list[0]["sunset"].as<char *>());
    moonrise = DateTimeConvert(dayly_list[0]["fxDate"].as<char *>(), dayly_list[0]["moonrise"].as<char *>());
    moonset = DateTimeConvert(dayly_list[0]["fxDate"].as<char *>(), dayly_list[0]["moonset"].as<char *>());
    log_i("sunrise:%s,sunset:%s,moonrise:%s,moonset:%s",
          sunrise.format("hh:mm"),
          sunset.format("hh:mm"),
          moonrise.format("hh:mm"),
          moonset.format("hh:mm"));
    return true;
  }
  bool FillMoon(const DynamicJsonDocument &root) //good
  {
    DynamicJsonDocument moon = root["moonPhase"];
    moonPhase = moon[0]["value"].as<float>();
    moonPhaseStr = moon[0]["name"].as<char *>();
    return true;
  }

public:
  Weather()
      : currentTime(), sunrise(), sunset(), moonrise(), moonset(), winddir(0), windspeed(0), maxRain(MIN_RAIN)
  {
    Clear();
  }

  /* Clear the internal data. */
  void Clear()
  {
  }

  /* Start the request and the filling. */
  bool Get()
  {
    DynamicJsonDocument doc(35 * 1024);
    String uri;

    uri = GetQWeatherAPIUri(API_NOW_URI);
    if (GetJsonDoc(uri, doc))
    {
      FillNow(doc);
    }
    else
      return false;

    doc.clear();
    uri = GetQWeatherAPIUri(API_24H_URI);
    if (GetJsonDoc(uri, doc))
    {
      Fill24h(doc);
      doc.clear();
    }
    else
      return false;

    doc.clear();
    uri = GetQWeatherAPIUri(API_7D_URI);
    if (GetJsonDoc(uri, doc))
    {
      Fill7d(doc);
    }
    else
      return false;
    
    doc.clear();

    String arg = "&date="+currentTime.format("YYYYMMDD");
    uri = GetQWeatherAPIUri(API_MOON_URI,arg.c_str());
    if (GetJsonDoc(uri, doc))
    {
      FillMoon(doc);
    }
    else
      return false;

    return true;
  }
};
