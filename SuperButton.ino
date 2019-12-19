/*************************************************************
   "SuperButton" Assistive Technology Button

   Provides a simple dry-contact output assistive-technology
   button using a load cell, so that the force required to activate
   the button can be adjusted to suit the needs of the user.

   More information:
     www.superhouse.tv/sb

   To do:
    - Set zero point on startup.
    - Require button press for 1 second to enter adjustment mode.
    - Debounce button.

   By:
    Chris Fryer <chris.fryer78@gmail.com>
    Jonathan Oxer <jon@oxer.com.au>

   Copyright 2019 SuperHouse Automation Pty Ltd www.superhouse.tv
 *************************************************************/
#include "config.h"

// For load cell amplifier:
#include "HX711.h"
int16_t zero_pressure_offset = 5;

// For OLED:
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 7
Adafruit_SSD1306 display(OLED_RESET);

// For rotary encoder:
#include <Encoder.h>  // "Encoder" library by PJRC
long old_encoder_position = -999;
long last_activity_time   = 0;

// For non-volatile memory:
#include <EEPROM.h>
int eeprom_address = 0;

/* 227 (Pocophone) = 920000 */

// Load cell connections
const int LOADCELL_DOUT_PIN = 5;
const int LOADCELL_SCK_PIN  = 4;
const int TRIGGER_LEVEL_PIN = A0;
int trigger_level   = DEFAULT_TRIGGER_LEVEL;
int pressure_level  =  0;
bool adjust_mode    = false;
bool button_pressed = false;

// Rotary encoder connections
#define ENCODER_PIN_A    1
#define ENCODER_PIN_B    0
#define ENCODER_SWITCH  A0

// Output connections
#define LED_PIN         13
#define HAPTIC_PIN      12
#define OUTPUT_PIN       6

// Create load cell object
HX711 scale;

// Create rotary encoder object
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

void check_current_pressure();
void check_ui_timeout();
void read_rotary_encoder();
void update_display();
void read_load_cell();
void check_button();

/**
   Setup
*/
void setup() {
  Serial.begin(9600);
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  Serial.println("Starting");

  pinMode(ENCODER_SWITCH, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A,  INPUT_PULLUP);
  pinMode(ENCODER_PIN_B,  INPUT_PULLUP);

  pinMode(LED_PIN,    OUTPUT);
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);

  // Load the trigger level from EEPROM if possible, or fall back to default
  EEPROM.get(eeprom_address, trigger_level);
  if (trigger_level < 1 || trigger_level > 100)
  {
    trigger_level = DEFAULT_TRIGGER_LEVEL;
  }

  // Initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Button not connected");
  display.display();

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // We only reach the next line if scale.begin() succeeded. If it didn't,
  // the controller stalls with "Button not connected" on the display.
  // If it succeeded, that message is very quickly wiped below.
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Ready");
  display.display();
}


/**
   Loop
*/
void loop() {
  read_load_cell();
  check_current_pressure();
  check_ui_timeout();
  read_rotary_encoder();
  update_display();
  check_button();
}


/*
   See if the current reading from the load cell exceeds the trigger level
*/
void check_current_pressure()
{
  if (pressure_level > trigger_level)
  {
    Serial.println("              ON");
    digitalWrite(LED_PIN, HIGH);
    if (ENABLE_HAPTIC_FEEDBACK)
    {
      digitalWrite(HAPTIC_PIN, HIGH);
    }
    digitalWrite(OUTPUT_PIN, HIGH);
  } else {
    Serial.println();
    digitalWrite(LED_PIN, LOW);
    if (ENABLE_HAPTIC_FEEDBACK)
    {
      digitalWrite(HAPTIC_PIN, LOW);
    }
    digitalWrite(OUTPUT_PIN, LOW);
  }
}

/**
   See if the UI has timed out
*/
void check_ui_timeout()
{
  if (millis() - last_activity_time > ADJUSTMENT_MODE_TIMEOUT)
  {
    adjust_mode = false;
  }
}

/**
   Check activity on the rotary encoder
*/
void read_rotary_encoder()
{
  // Ignore the rotary encoder unless we're in adjustment mode
  if (adjust_mode == true)
  {
    long new_encoder_position = encoder.read();
    if ( new_encoder_position > old_encoder_position)
    {
      trigger_level++;
      last_activity_time = millis();
      // Prevent the trigger level going above 100%
      if (trigger_level > 100)
      {
        trigger_level = 100;
      }
      old_encoder_position = new_encoder_position;
    }
    if ( new_encoder_position < old_encoder_position)
    {
      trigger_level--;
      last_activity_time = millis();
      // Prevent the trigger level going below 1%
      if (trigger_level < 1)
      {
        trigger_level = 1;
      }
      old_encoder_position = new_encoder_position;
    }
  } else {
    // Even if we don't want to read the encoder, consume the value
    // from it so that a queued change doesn't get applied the moment
    // we enter adjustment mode
    old_encoder_position = encoder.read();
  }
}

/**
   Draw current values on the OLED
*/
void update_display()
{
  // Display the current pressure level
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 2);
  display.println(pressure_level);

  // Dividing line between numbers
  display.drawLine(67, 0, 67, 31, WHITE);

  // Display the trigger level
  display.setTextSize(3);
  if (adjust_mode == true)
  {
    display.setTextColor(BLACK, WHITE);
    display.fillRect(67, 0, 127, 32, WHITE);
  } else {
    display.setTextColor(WHITE);
  }
  display.setCursor(73, 5);
  display.println(trigger_level);
  display.display();
}

/**
   Read the current pressure level
*/
void read_load_cell()
{
  if (scale.is_ready()) {
    long pressure_reading = scale.read() * -1;
    pressure_level    = map(pressure_reading, 0, 1000000, 0, 100) + zero_pressure_offset;
    Serial.print("Pressure: ");
    Serial.print(pressure_level);
    Serial.print("    Trigger: ");
    Serial.print(trigger_level);
  } else {
    Serial.println("HX711 not found.");
  }
}

/**
   See if the rotary encoder button has been pressed
*/
void check_button()
{
  byte button_state = digitalRead(ENCODER_SWITCH);
  if (button_state == LOW && button_pressed == false)
  {
    // The button has transitioned from off to on
    last_activity_time = millis();
    button_pressed = true;
    if (adjust_mode == true)
    {
      adjust_mode = false;
      Serial.println("Lock");
      EEPROM.put(eeprom_address, trigger_level);
    } else {
      adjust_mode = true;
      Serial.println("Adjust");
    }
  }
  if (button_state == HIGH && button_pressed == true)
  {
    // The button has transitioned from on to off
    button_pressed = false;
  }
}
