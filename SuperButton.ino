/*************************************************************
   "SuperButton" Assistive Technology Button

   Provides a simple dry-contact output assistive-technology
   button using a load cell, so that the force required to activate
   the button can be adjusted to suit the needs of the user.

   More information:
     www.superhouse.tv/sb

   To do:
    - Require button press for 1 second to enter adjustment mode.
    - Debounce button.

   By:
    Chris Fryer <chris.fryer78@gmail.com>
    Jonathan Oxer <jon@oxer.com.au>

   Copyright 2019-2020 SuperHouse Automation Pty Ltd www.superhouse.tv
 *************************************************************/
/*--------------------------- Configuration ------------------------------*/
// Configuration should be done in the included file:
#include "config.h"

/*--------------------------- Libraries ----------------------------------*/
// For load cell amplifier:
#include "HX711.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>              // "Encoder" library by PJRC
#include <EEPROM.h>
/*--------------------------- Global Variables ---------------------------*/
// OLED:
uint32_t g_last_screen_update   = 0;

// Rotary encoder:
int32_t  g_old_encoder_position = -999;
uint32_t g_last_activity_time   = 0;

// Non-volatile memory:
int g_eeprom_address = 0;

/* 227 (Pocophone) = 920000 */
// Load cell:
int32_t g_trigger_level         = DEFAULT_TRIGGER_LEVEL;
int32_t g_pressure_level        =  0;
uint8_t g_adjust_mode           = false;
uint8_t g_button_pressed        = false;
int32_t g_zero_pressure_offset  = 0;

/*--------------------------- Function Signatures ---------------------------*/
void checkIfThresholdReached();
void readPressureLevel();
int  getScaledLoadSensorValue();
void checkUiTimeout();
void readRotaryEncoder();
void updateOledDisplay();
void readLoadCell();
void checkButton();

/*--------------------------- Instantiate Global Objects --------------------*/
// Load cell
HX711 scale;

// Rotary encoder
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

// OLED display
Adafruit_SSD1306 display(128, 32, &Wire, -1);

/*--------------------------- Program ---------------------------------------*/
/**
  Setup
*/
void setup() {
  Serial.begin(115200);
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
  EEPROM.get(g_eeprom_address, g_trigger_level);
  if (g_trigger_level < 1 || g_trigger_level > 100)
  {
    g_trigger_level = DEFAULT_TRIGGER_LEVEL;
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
  delay(100); // Load cell requires ~15ms to settle. Allow longer for safety.

  // Read the load cell to set the zero offset. This is like
  // an automatic "tare" operation at startup
  g_zero_pressure_offset = -1 * getScaledLoadCellValue();
  Serial.print("Tare offset: ");
  Serial.println(g_zero_pressure_offset, DEC);

  // This vvvvvvvvvvvvvv doesn't seem to work. Always gets to this point!
  //
  // We only reach the next line if scale.begin() succeeded above. If it
  // didn't, the controller stalls with "Button not connected" on the display.
  // If it succeeded, that message is very quickly wiped below.
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Ready");
  display.display();
}

/**
  Loop
*/
void loop()
{
  readPressureLevel();
  checkIfThresholdReached();
  checkUiTimeout();
  readRotaryEncoder();
  updateOledDisplay();
  checkButton();
}

/**
  See if the current reading from the load cell exceeds the trigger level
*/
void checkIfThresholdReached()
{
  if (g_pressure_level > g_trigger_level)
  {
    if (ENABLE_SERIAL_DEBUGGING)
    {
      Serial.println("              ON");
    }
    digitalWrite(LED_PIN, HIGH);
    if (ENABLE_HAPTIC_FEEDBACK)
    {
      digitalWrite(HAPTIC_PIN, HIGH);
    }
    digitalWrite(OUTPUT_PIN, HIGH);
    g_last_activity_time = millis();
  } else {
    if (ENABLE_SERIAL_DEBUGGING)
    {
      Serial.println();
    }
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
void checkUiTimeout()
{
  if (millis() - g_last_activity_time > (ADJUSTMENT_MODE_TIMEOUT * 1000))
  {
    g_adjust_mode = false;
  }
}

/**
  Check activity on the rotary encoder
*/
void readRotaryEncoder()
{
  int32_t new_encoder_position = 0;
  // Ignore the rotary encoder unless we're in adjustment mode
  if (true == g_adjust_mode)
  {
    new_encoder_position = encoder.read();
    if ( new_encoder_position > g_old_encoder_position)
    {
      g_trigger_level++;
      g_last_activity_time = millis();
      // Prevent the trigger level going above 100%
      if (g_trigger_level > 100)
      {
        g_trigger_level = 100;
      }
      g_old_encoder_position = new_encoder_position;
    }
    if ( new_encoder_position < g_old_encoder_position)
    {
      g_trigger_level--;
      g_last_activity_time = millis();
      // Prevent the trigger level going below 1%
      if (g_trigger_level < 1)
      {
        g_trigger_level = 1;
      }
      g_old_encoder_position = new_encoder_position;
    }
  } else {
    // Even if we don't want to read the encoder, consume the value
    // from it so that a queued change doesn't get applied the moment
    // we enter adjustment mode. We also use this to un-blank the
    // display if the knob is twiddled.
    new_encoder_position = encoder.read();
    if (new_encoder_position != g_old_encoder_position)
    {
      g_old_encoder_position = new_encoder_position;
      g_last_activity_time = millis();
    }
  }
}

/**
  Draw current values on the OLED
*/
void updateOledDisplay()
{
  uint32_t time_now = millis();

  if (time_now - g_last_screen_update > SCREEN_UPDATE_INTERVAL)
  {
    g_last_screen_update = time_now;

    if (time_now - g_last_activity_time < (SCREEN_TIMEOUT_INTERVAL * 1000))
    {
      // The screen hasn't timed out yet, so display stuff
      // Display the current pressure level
      display.clearDisplay();
      display.setTextSize(4);
      //display.setTextFont(6);
      display.cp437(true);
      display.setTextColor(WHITE, BLACK);
      display.setCursor(0, 2);
      display.println(g_pressure_level);

      // Dividing line between numbers
      display.drawLine(67, 0, 67, 31, WHITE);

      // Display the trigger level
      display.setTextSize(3);
      if (true == g_adjust_mode)
      {
        display.setTextColor(BLACK, WHITE);
        display.fillRect(67, 0, 127, 32, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(73, 5);
      display.println(g_trigger_level);
      display.display();
      if (ENABLE_SERIAL_DEBUGGING)
      {
        Serial.println("Show");
      }
    } else {
      // The screen has timed out, so blank it
      display.clearDisplay();
      display.display();
      if (ENABLE_SERIAL_DEBUGGING)
      {
        Serial.println("Blank");
      }
    }
  }
}

/**
  Read the current pressure level
*/
void readPressureLevel()
{
  g_pressure_level = getScaledLoadCellValue() + g_zero_pressure_offset;
  if (ENABLE_SERIAL_DEBUGGING)
  {
    Serial.println(g_pressure_level);
  }
}

/**
  Read the load cell and scale the value to a percentage
*/
int32_t getScaledLoadCellValue()
{
  int32_t pressure;
  if (scale.is_ready())
  {
    int32_t pressure_reading = scale.read() * -1;
    pressure = map(pressure_reading, 0, 1000000, 0, 100);
  } else {
    //Serial.println("HX711 not found.");
  }
  return pressure;
}

/**
  See if the rotary encoder button has been pressed
*/
void checkButton()
{
  uint8_t button_state = digitalRead(ENCODER_SWITCH);
  if (LOW == button_state && false == g_button_pressed)
  {
    // The button has transitioned from off to on
    g_last_activity_time = millis();
    g_button_pressed = true;
    if (true == g_adjust_mode)
    {
      g_adjust_mode = false;
      if (ENABLE_SERIAL_DEBUGGING)
      {
        Serial.println("Lock");
      }
      EEPROM.put(g_eeprom_address, g_trigger_level);
    } else {
      g_adjust_mode = true;
      if (ENABLE_SERIAL_DEBUGGING)
      {
        Serial.println("Adjust");
      }
    }
  }
  if (HIGH == button_state && true == g_button_pressed)
  {
    // The button has transitioned from on to off
    g_last_activity_time = millis();
    g_button_pressed = false;
  }
}
