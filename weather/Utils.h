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
  * @file Utils.h
  * 
  * A collection of helper functions.
  */
#pragma once

/* Convert the rssi value to a string value between 0 and 100 % */
String WifiGetRssiAsQuality(int rssi)
{
   int quality = 0;

   if (rssi <= -100) {
      quality = 0;
   } else if (rssi >= -50) {
      quality = 100;
   } else {
      quality = 2 * (rssi + 100);
   }
   return String(quality);
}

/* Convert the rssi value to a int value between 0 and 100 % */
int WifiGetRssiAsQualityInt(int rssi)
{
   int quality = 0;

   if (rssi <= -100) {
      quality = 0;
   } else if (rssi >= -50) {
      quality = 100;
   } else {
      quality = 2 * (rssi + 100);
   }
   return quality;
}

/* Convert a day, month, year to a julian int
 * The moon phase calculation is part of the github project
 * https://github.com/G6EJD/ESP32-Revised-Weather-Display-42-E-Paper
 * See http://www.dsbird.org.uk
 * Copyright (c) David Bird
 */
int JulianDate(int d, int m, int y) 
{
   int mm, yy, k1, k2, k3, j;
   
   yy = y - (int)((12 - m) / 10);
   mm = m + 9;
   if (mm >= 12) mm = mm - 12;
   k1 = (int)(365.25 * (yy + 4712));
   k2 = (int)(30.6001 * mm + 0.5);
   k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
   // 'j' for dates in Julian calendar:
   j = k1 + k2 + d + 59 + 1;
   if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
   return j;
} 

/* Normalize the moon phase with the julian date format
 * The moon phase calculation is part of the github project
 * https://github.com/G6EJD/ESP32-Revised-Weather-Display-42-E-Paper
 * See http://www.dsbird.org.uk
 * Copyright (c) David Bird
 */
double NormalizedMoonPhase(int d, int m, int y) 
{
   int    j     = JulianDate(d, m, y);
   double Phase = (j + 4.867) / 29.53059;
   
   return (Phase - (int) Phase);
}
