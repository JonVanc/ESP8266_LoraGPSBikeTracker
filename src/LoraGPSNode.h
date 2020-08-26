#ifndef MAIN_H
#define MAIN_H

#define WEMOS_LORA_GW

#include <Arduino.h>
#include <lmic.h> //Install "LMIC-Arduino by IBM" (NOT "MCCI LoRaWAN LMIC library by IBM") in Platformio_Icon->QuickAccess->PIO_Home->Libraries
#include <hal/hal.h>
#include <SPI.h>
#include <SoftwareSerial.h> //For GPS
#include <TinyGPS++.h>
#include <credentials.h>
#include "../lib/RGBLed.cpp"

String getGPSData();

#endif