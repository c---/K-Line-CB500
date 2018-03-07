/*
   Copyright (C) 2018 Chris Osgood

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

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define CMD_WAIT 100

static uint32_t last = 0;

Adafruit_PCD8544 display = Adafruit_PCD8544(4, 5, 6);

static byte cksum(const byte* data, byte len)
{
  byte rv = 0;
  for (byte i = 0; i < len; i++) rv -= data[i];
  return rv;
}

static void flush_serial()
{
  while (Serial.read() != -1);
}

static bool do_cmd(byte* cmd, byte len, byte* resp = 0, byte* rlen = 0)
{   
  byte buf[256];
  byte c, try_cnt = 0;

  cmd[1] = len;
  cmd[len - 1] = cksum(cmd, len - 1);

docmd:
  uint32_t tm = millis() - last;
  if (tm < CMD_WAIT)
    delay(CMD_WAIT - tm);

  flush_serial();
  
  Serial.write(cmd, len);
  Serial.flush();

  last = millis();

  c = Serial.readBytes(buf, len);
  if (c != len)
    goto tryagain;
   
  if (resp)
  {
    c = Serial.readBytes(resp, 2);
    if (c != 2)
      goto tryagain;
    
    c = Serial.readBytes(&resp[c], buf[1] - 2);
    if (c != resp[1])
      goto tryagain;

    if (cksum(resp, resp[1] - 1) != resp[resp[1] - 1])
      goto tryagain;
  }

  return true;

tryagain:
  try_cnt++;
  if (try_cnt < 3)
    goto docmd;

  return false;
}

static bool do_init(void)
{
  static byte wake[] = { 0xFE, 0x00, 0xFF, 0x00 };
  static byte initial[] = { 0x72, 0x00, 0x00, 0xF0, 0x00 };

  byte buf[256], len;
  
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW); // Pull low
  delay(70);
  pinMode(2, INPUT); // Let high
  delay(120);
  do_cmd(wake, sizeof(wake));
  delay(200);
  return do_cmd(initial, sizeof(initial), buf, &len);
}

void setup()
{
  pinMode(9, INPUT_PULLUP);
  
  display.begin();
  display.setContrast(60);
  display.clearDisplay();

  Serial.begin(10400);
  Serial.setTimeout(100);
}

void loop()
{
  if (!digitalRead(9))
  {
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0, 0);
    display.print("Init:      ");
    display.setCursor(6, 0);
    
    while (!digitalRead(9));
    
    display.println(do_init() ? "true" : "false");
    display.display();
  }
}

