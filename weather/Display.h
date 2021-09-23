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
  * @file Display.h
  * 
  * Main class for drawing the content to the e-paper display.
  */
#pragma once
#include "Data.h"
#include "Icons.h"
#include <M5EPD.h>

#define FONT_SIZE_1 12
#define FONT_SIZE_2 19
#define FONT_SIZE_3 26
#define FONT_SIZE_4 28

M5EPD_Canvas canvas(&M5.EPD); // Main canvas of the e-paper

/* Main class for drawing the content to the e-paper display. */
class WeatherDisplay
{
protected:
   MyData &myData; //!< Reference to the global data
   int maxX;       //!< Max width of the e-paper
   int maxY;       //!< Max height of the e-paper

protected:
   void DrawCircle(int32_t x, int32_t y, int32_t r, uint32_t color, int32_t degFrom = 0, int32_t degTo = 360);
   void Arrow(int x, int y, int asize, int aangle, int pwidth, int plength);
   void DisplayDisplayWindSection(int x, int y, int angle, int windspeed, int windscale, String windDirStr, int radius);

   void DrawIcon(int x, int y, const uint16_t *icon, int dx = 64, int dy = 64, bool highContrast = false);
   void DrawIcon(int x, int y, String icon, int dx = 64, int dy = 64);
   void DrawMoon(int x, int y, double moonPhase);

   void DrawHead();
   void DrawRSSI(int x, int y);
   void DrawBattery(int x, int y);

   void DrawAstronomyInfo(int x, int y, int dx, int dy);
   void DrawCurrentWeather(int x, int y, int dx, int dy);
   void DrawWindInfo(int x, int y, int dx, int dy);
   void DrawM5PaperInfo(int x, int y, int dx, int dy);

   void DrawHourly(int x, int y, int dx, int dy, Weather &weather, int index);

   void DrawGraph(int x, int y, int dx, int dy, String title, int xMin, int xMax, int yMin, int yMax, float values[]);

public:
   WeatherDisplay(MyData &md, int x = 960, int y = 540)
       : myData(md), maxX(x), maxY(y)
   {
   }
   void LoadFont(String filename);

   void Show();

   void ShowM5PaperInfo();
};
void WeatherDisplay::LoadFont(String path)
{
   if (SD.exists(path))
   {
      esp_err_t error = canvas.loadFont(path, SD);
      log_d("ret:%s", esp_err_to_name(error));
      if (ESP_OK == error)
      {
         canvas.useFreetypeFont(true);
         canvas.createRender(FONT_SIZE_1);
         canvas.createRender(FONT_SIZE_2);
         canvas.createRender(FONT_SIZE_3);
         canvas.createRender(FONT_SIZE_4);
      }
   }
   else
   {
      log_d("ret:%s", "Font not found!");
   }
}

/* Draw a circle with optional start and end point */
void WeatherDisplay::DrawCircle(int32_t x, int32_t y, int32_t r, uint32_t color, int32_t degFrom /* = 0 */, int32_t degTo /* = 360 */)
{
   for (int i = degFrom; i < degTo; i++)
   {
      double radians = i * PI / 180;
      double px = x + r * cos(radians);
      double py = y + r * sin(radians);

      canvas.drawPixel(px, py, color);
   }
}

/* Draw a the rssi value as circle parts */
void WeatherDisplay::DrawRSSI(int x, int y)
{
   int iQuality = WifiGetRssiAsQualityInt(myData.wifiRSSI);

   if (iQuality >= 80)
      DrawCircle(x + 12, y, 16, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 40)
      DrawCircle(x + 12, y, 12, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 20)
      DrawCircle(x + 12, y, 8, M5EPD_Canvas::G15, 225, 315);
   if (iQuality >= 10)
      DrawCircle(x + 12, y, 4, M5EPD_Canvas::G15, 225, 315);
   DrawCircle(x + 12, y, 2, M5EPD_Canvas::G15, 225, 315);
}

/* Draw a the battery icon */
void WeatherDisplay::DrawBattery(int x, int y)
{
   canvas.drawRect(x, y, 30, 16, M5EPD_Canvas::G15);
   canvas.drawRect(x + 30, y + 3, 4, 10, M5EPD_Canvas::G15);
   for (int i = x; i < x + 30; i++)
   {
      canvas.drawLine(i, y, i, y + 15, M5EPD_Canvas::G15);
      if ((i - x) * 100.0 / 30.0 > myData.batteryCapacity)
      {
         break;
      }
   }
}

/* Draw a the head with version, city, rssi and battery */
void WeatherDisplay::DrawHead()
{
   canvas.drawString(VERSION, 20, 10);
   canvas.drawCentreString(CITY_NAME, maxX / 2, 10, 1);
   canvas.drawRightString(WifiGetRssiAsQuality(myData.wifiRSSI) + "%", maxX - 160, 10,1);
   DrawRSSI(maxX - 155, 25);
   canvas.drawRightString(String(myData.batteryCapacity) + "%", maxX - 60, 10,1);
   DrawBattery(maxX - 50, 10);
}

/* Draw one icon from the binary data */
void WeatherDisplay::DrawIcon(int x, int y, const uint16_t *icon, int dx /*= 64*/, int dy /*= 64*/, bool highContrast /*= false*/)
{
   for (int yi = 0; yi < dy; yi++)
   {
      for (int xi = 0; xi < dx; xi++)
      {
         uint16_t pixel = icon[yi * dx + xi];

         if (highContrast)
         {
            if (15 - (pixel / 4096) > 0)
               canvas.drawPixel(x + xi, y + yi, M5EPD_Canvas::G15);
         }
         else
         {
            canvas.drawPixel(x + xi, y + yi, 15 - (pixel / 4096));
         }
      }
   }
}

void WeatherDisplay::DrawIcon(int x, int y, String icon, int dx, int dy)
{
   String path = "/weather_icons/" + icon + ".png";
   canvas.drawPngFile(SD, path.c_str(), x, y, dx, dy, 0, 0, 1.0, 127);
}

/* Draw the sun information with sunrise and sunset */
void WeatherDisplay::DrawAstronomyInfo(int x, int y, int dx, int dy)
{
   canvas.setTextSize(FONT_SIZE_4);
   canvas.drawCentreString("天文天像", x + dx / 2, y + 7, 1);
   canvas.drawLine(x, y + 35, x + dx, y + 35, M5EPD_Canvas::G15);

   canvas.setTextSize(FONT_SIZE_4);
   DrawIcon(x + 25, y + 40, (uint16_t *)SUNRISE64x64);
   canvas.drawString(myData.weather.sunrise.format("hh:mm"), x + 105, y + 65, 1);

   DrawIcon(x + 25, y + 105, (uint16_t *)SUNSET64x64);
   canvas.drawString(myData.weather.sunset.format("hh:mm"), x + 105, y + 130, 1);
   
   canvas.setTextSize(FONT_SIZE_3);
   DrawMoon(x + 12, y + 160, myData.weather.moonPhase);
   canvas.drawString(myData.weather.moonPhaseStr, x + 105, y + 195, 1);
}

/* The moon phase drawing was from the github project
 * https://github.com/G6EJD/ESP32-Revised-Weather-Display-42-E-Paper
 * See http://www.dsbird.org.uk
 * Copyright (c) David Bird
 */
void WeatherDisplay::DrawMoon(int x, int y, double moonPhase)
{
   const int diameter = 45;
   const int number_of_lines = 90;
   double Phase = myData.weather.moonPhase;
   log_d("moonPhase:%f", Phase);

   canvas.drawCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, M5EPD_Canvas::G15);

   for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++)
   {
      double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);

      double Rpos = 2 * Xpos;
      double Xpos1, Xpos2;

      if (Phase < 0.5)
      {
         Xpos1 = -Xpos;
         Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
      }
      else
      {
         Xpos1 = Xpos;
         Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
      }
      double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
      double pW1y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
      double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
      double pW2y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
      double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
      double pW3y = (Ypos + number_of_lines) / number_of_lines * diameter + y;
      double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
      double pW4y = (Ypos + number_of_lines) / number_of_lines * diameter + y;

      canvas.drawLine(pW1x, pW1y, pW2x, pW2y, M5EPD_Canvas::G15);
      canvas.drawLine(pW3x, pW3y, pW4x, pW4y, M5EPD_Canvas::G15);
   }
   canvas.drawCircle(x + diameter - 1, y + diameter, diameter / 2, M5EPD_Canvas::G15);
}

/* Draw the moon information with moonrise, moonset and moon phase */
void WeatherDisplay::DrawCurrentWeather(int x, int y, int dx, int dy)
{
   canvas.setTextSize(FONT_SIZE_4);
   canvas.drawCentreString("实时天气", x + dx / 2, y + 7, 1);
   canvas.drawLine(x, y + 35, x + dx, y + 35, M5EPD_Canvas::G15);

   canvas.setTextSize(FONT_SIZE_3);
   //DrawIcon(x + 30, y + 40, (uint16_t *)MOONRISE64x64);
   DrawIcon(x + 30, y + 40, myData.weather.currentIcon);
   canvas.drawString(myData.weather.currentText, x + 110, y + 65, 1);

   canvas.setTextSize(FONT_SIZE_4);
   DrawIcon(x + 30, y + 105, (uint16_t *)TEMPERATURE64x64);
   canvas.drawString(String(myData.weather.currentTemp)+" ℃", x + 110, y + 130, 1);

   DrawIcon(x + 30, y + 170, (uint16_t *)HUMIDITY64x64);
   canvas.drawString(String(myData.weather.currentHumidity)+"%", x + 110, y + 195, 1);

}

/* Draw the in the wind section
 * The wind section drawing was from the github project
 * https://github.com/G6EJD/ESP32-Revised-Weather-Display-42-E-Paper
 * See http://www.dsbird.org.uk
 * Copyright (c) David Bird
 */
void WeatherDisplay::Arrow(int x, int y, int asize, int aangle, int pwidth, int plength)
{
   float dx = (asize + 21) * cos((aangle - 90) * PI / 180) + x; // calculate X position
   float dy = (asize + 21) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
   float x1 = 0;
   float y1 = plength;
   float x2 = pwidth / 2;
   float y2 = pwidth / 2;
   float x3 = -pwidth / 2;
   float y3 = pwidth / 2;
   float angle = aangle * PI / 180;
   float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
   float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
   float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
   float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
   float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
   float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
   canvas.fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, M5EPD_Canvas::G15);
}

/* Draw the wind circle with the windspeed data
 * The wind section drawing was from the github project
 * https://github.com/G6EJD/ESP32-Revised-Weather-Display-42-E-Paper
 * See http://www.dsbird.org.uk
 * Copyright (c) David Bird
 */
void WeatherDisplay::DisplayDisplayWindSection(int x, int y, int angle, int windspeed, int windscale, String windDirStr, int cradius)
{
   int dxo, dyo, dxi, dyi;

   canvas.setTextSize(FONT_SIZE_2);
   canvas.drawLine(0, 15, 0, y + cradius + 30, M5EPD_Canvas::G15);
   canvas.drawCircle(x, y, cradius, M5EPD_Canvas::G15);       // Draw compass circle
   canvas.drawCircle(x, y, cradius + 1, M5EPD_Canvas::G15);   // Draw compass circle
   canvas.drawCircle(x, y, cradius * 0.7, M5EPD_Canvas::G15); // Draw compass inner circle
   for (float a = 0; a < 360; a = a + 22.5)
   {
      dxo = cradius * cos((a - 90) * PI / 180);
      dyo = cradius * sin((a - 90) * PI / 180);
      if (a == 45)
         canvas.drawCentreString("东北", dxo + x + 20, dyo + y - 20, 1);
      if (a == 135)
         canvas.drawCentreString("东南", dxo + x + 20, dyo + y + 10, 1);
      if (a == 225)
         canvas.drawCentreString("西南", dxo + x - 20, dyo + y + 10, 1);
      if (a == 315)
         canvas.drawCentreString("西北", dxo + x - 20, dyo + y - 20, 1);
      dxi = dxo * 0.9;
      dyi = dyo * 0.9;
      canvas.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, M5EPD_Canvas::G15);
      dxo = dxo * 0.7;
      dyo = dyo * 0.7;
      dxi = dxo * 0.9;
      dyi = dyo * 0.9;
      canvas.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, M5EPD_Canvas::G15);
   }
   canvas.drawCentreString("北", x, y - cradius - 24, 1);
   canvas.drawCentreString("南", x, y + cradius + 8, 1);
   canvas.drawCentreString("西", x - cradius - 15, y - 5, 1);
   canvas.drawCentreString("东", x + cradius + 15, y - 5, 1);
   canvas.drawCentreString(String(windspeed) + " km/h", x, y - 18, 1);
   canvas.drawCentreString(windDirStr + String(windscale) + "级", x, y + 2, 1);

   Arrow(x, y, cradius - 17, angle, 15, 27);
}

/* Draw the wind information part */
void WeatherDisplay::DrawWindInfo(int x, int y, int dx, int dy)
{
   canvas.setTextSize(FONT_SIZE_4);
   canvas.drawCentreString("风力风向", x + dx / 2, y + 7, 1);
   canvas.drawLine(x, y + 35, x + dx, y + 35, M5EPD_Canvas::G15);

   DisplayDisplayWindSection(x + dx / 2, y + dy / 2 + 20,
                             myData.weather.winddir,
                             myData.weather.windspeed,
                             myData.weather.windscale,
                             myData.weather.windDirStr,
                             75);
}

/* Draw the M5Paper environment and RTC information */
void WeatherDisplay::DrawM5PaperInfo(int x, int y, int dx, int dy)
{
   canvas.setTextSize(FONT_SIZE_4);
   canvas.drawCentreString("M5Paper", x + dx / 2, y + 7, 1);
   canvas.drawLine(x, y + 35, x + dx, y + 35, M5EPD_Canvas::G15);

   canvas.setTextSize(FONT_SIZE_4);
   String date = String(myData.weather.currentTime.format("YYYY.MM.DD"));
   String week = myData.weather.dayOfWeek[myData.weather.currentTime.dayOfWeek()];
   canvas.drawCentreString(date + " " + week, x + dx / 2, y + 55, 1);
   canvas.drawCentreString(String(myData.weather.currentTime.format("hh:mm")), x + dx / 2, y + 95, 1);
   canvas.setTextSize(FONT_SIZE_2);
   canvas.drawCentreString("updated", x + dx / 2, y + 120, 1);

   canvas.setTextSize(FONT_SIZE_4);
   DrawIcon(x + 35, y + 140, (uint16_t *)TEMPERATURE64x64);
   canvas.drawString(String(myData.sht30Temperatur) + " ℃", x + 35, y + 210, 1);
   DrawIcon(x + 145, y + 140, (uint16_t *)HUMIDITY64x64);
   canvas.drawString(String(myData.sht30Humidity) + "%", x + 150, y + 210, 1);
}

/* Draw one hourly weather information */
void WeatherDisplay::DrawHourly(int x, int y, int dx, int dy, Weather &weather, int index)
{
   DateTime time = weather.hourlyTime[index];
   int temp = weather.hourlyTemp[index];
   String main = weather.hourlyMain[index];
   String icon = weather.hourlyIcon[index];
   String text = weather.hourlyText[index];

   canvas.setTextSize(FONT_SIZE_2);
   canvas.drawCentreString(String(time.hour()) + ":00", x + dx / 2, y + 10, 1);
   canvas.drawCentreString(text+" "+String(temp) + "℃", x + dx / 2, y + 30, 1);
   // canvas.drawCentreString(main,                        x + dx / 2, y + 70, 1);

   int iconX = x + dx / 2 - 32;
   int iconY = y + 50;

   // DrawIcon(x + dx / 2 - 32, y + 50, (uint16_t *) image_data_03d, 64, 64, true);

   String path = "/weather_icons/" + icon + ".png";
   canvas.drawPngFile(SD, path.c_str(), iconX, iconY, 64, 64, 0, 0, 1.0, 127);
}

/* Draw a graph with x- and y-axis and values */
void WeatherDisplay::DrawGraph(int x, int y, int dx, int dy, String title, int xMin, int xMax, int yMin, int yMax, float values[])
{
   String yMinString = String(yMin);
   String yMaxString = String(yMax);
   int textWidth = 5 + max(yMinString.length() * 3.5, yMaxString.length() * 3.5);
   //int textWidth = 12;
   int graphX = x + 10 + textWidth;
   int graphY = y + 35;
   int graphDX = dx - textWidth - 20;
   int graphDY = dy - 35 - 20;
   float xStep = graphDX / (xMax - xMin);
   float yStep = graphDY / (yMax - yMin);
   int iOldX = 0;
   int iOldY = 0;

   canvas.setTextSize(FONT_SIZE_2);
   canvas.drawCentreString(title, x + dx / 2, y + 10, 1);
   canvas.setTextSize(FONT_SIZE_1);
   canvas.drawRightString(yMaxString, graphX - 5, graphY - 5, 1);
   canvas.drawRightString(yMinString, graphX - 5, graphY + graphDY - 3, 1);

   for (int i = 0; i <= (xMax - xMin); i++)
   {
      //canvas.drawCentreString(String(i), graphX + i * xStep, graphY + graphDY + 5,1);
      canvas.drawCentreString(myData.weather.forecastDate[i], graphX + i * xStep, graphY + graphDY + 5, 1);
   }

   canvas.drawRect(graphX, graphY, graphDX, graphDY, M5EPD_Canvas::G15);
   if (yMin < 0 && yMax > 0)
   { // null line?
      float yValueDX = (float)graphDY / (yMax - yMin);
      int yPos = graphY + graphDY - (0.0 - yMin) * yValueDX;

      if (yPos > graphY + graphDY)
         yPos = graphY + graphDY;
      if (yPos < graphY)
         yPos = graphY;

      canvas.drawString("0", graphX - 20, yPos);
      for (int xDash = graphX; xDash < graphX + graphDX - 10; xDash += 10)
      {
         canvas.drawLine(xDash, yPos, xDash + 5, yPos, M5EPD_Canvas::G15);
      }
   }
   for (int i = xMin; i <= xMax; i++)
   {
      float yValue = values[i - xMin];
      float yValueDY = (float)graphDY / (yMax - yMin);
      int xPos = graphX + graphDX / (xMax - xMin) * i;
      int yPos = graphY + graphDY - (yValue - yMin) * yValueDY;

      if (yPos > graphY + graphDY)
         yPos = graphY + graphDY;
      if (yPos < graphY)
         yPos = graphY;

      canvas.fillCircle(xPos, yPos, 2, M5EPD_Canvas::G15);
      if (i > xMin)
      {
         canvas.drawLine(iOldX, iOldY, xPos, yPos, M5EPD_Canvas::G15);
      }
      iOldX = xPos;
      iOldY = yPos;
   }
}

/* Main function to show all the data to the e-paper */
void WeatherDisplay::Show()
{
   Serial.println("WeatherDisplay::Show");

   canvas.createCanvas(960, 540);

   canvas.setTextSize(FONT_SIZE_2);
   canvas.setTextColor(WHITE, BLACK);
   canvas.setTextDatum(TL_DATUM);

   DrawHead();

   // x = 960 y = 540
   // 540 - oben 35 - unten 10 = 495

   canvas.drawRect(14, 34, maxX - 28, maxY - 43, M5EPD_Canvas::G15);

   canvas.drawRect(15, 35, maxX - 30, 251, M5EPD_Canvas::G15);
   canvas.drawLine(232, 35, 232, 286, M5EPD_Canvas::G15);
   canvas.drawLine(465, 35, 465, 286, M5EPD_Canvas::G15);
   canvas.drawLine(697, 35, 697, 286, M5EPD_Canvas::G15);
   DrawAstronomyInfo(15, 35, 232, 251);
   DrawCurrentWeather(232, 35, 232, 251);
   DrawWindInfo(465, 35, 232, 251);
   DrawM5PaperInfo(697, 35, 245, 251);

   canvas.drawRect(15, 286, maxX - 30, 122, M5EPD_Canvas::G15);
   for (int x = 15, i = 0; x <= 930; x += 116, i += 3)
   {
      canvas.drawLine(x, 286, x, 408, M5EPD_Canvas::G15);
      DrawHourly(x, 286, 116, 122, myData.weather, i);
   }

   canvas.drawRect(15, 408, maxX - 30, 122, M5EPD_Canvas::G15);
   DrawGraph(18, 408, 232, 122, "温度 (℃)", 0, 6, myData.weather.minTemp - 5, myData.weather.maxTemp + 5, myData.weather.forecastMaxTemp);
   DrawGraph(18, 408, 232, 122, "温度 (℃)", 0, 6, myData.weather.minTemp - 5, myData.weather.maxTemp + 5, myData.weather.forecastMinTemp);
   DrawGraph(250, 408, 232, 122, "降水量 (mm)", 0, 6, 0, myData.weather.maxRain, myData.weather.forecastRain);
   DrawGraph(480, 408, 232, 122, "湿度 (%)", 0, 6, 0, 100, myData.weather.forecastHumidity);
   DrawGraph(715, 408, 232, 122, "气压 (hPa)", 0, 6, myData.weather.minPressure - 10, myData.weather.minPressure + 10, myData.weather.forecastPressure);

   canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
   delay(1000);
}

/* Update only the M5Paper part of the global data */
void WeatherDisplay::ShowM5PaperInfo()
{
   Serial.println("WeatherDisplay::ShowM5PaperInfo");

   canvas.createCanvas(245, 251);

   canvas.setTextSize(FONT_SIZE_2);
   canvas.setTextColor(WHITE, BLACK);
   canvas.setTextDatum(TL_DATUM);

   canvas.drawRect(0, 0, 245, 251, M5EPD_Canvas::G15);
   DrawM5PaperInfo(0, 0, 245, 251);

   canvas.pushCanvas(697, 35, UPDATE_MODE_GC16);
   delay(1000);
}
