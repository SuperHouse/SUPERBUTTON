/**
   "SuperButton" Assistive Technology Button

   Provides a simple dry-contact output assistive-technology
   button using a load cell, so that the force required to activate
   the button can be adjusted to suit the needs of the user.

   The load cell reading is zeroed at startup, and can also be
   re-zeroed by pressing the "tare" button.

   The output duration can directly match the button press, or it
   can be "stretched" to allow even a quick tap to cause a longer
   beep output. The beep stretching switch activates this option.

   External dependencies. Install using Arduino library manager:
     "Adafruit GFX Library" by Adafruit
     "Adafruit SSD1306" by Adafruit
     "Encoder" library by Paul Stoffregen
     "HX711 Arduino Library" by Bogdan Necula, Andreas Motti

   More information:
     www.superhouse.tv/sb

   To do:
    - Require button press for 1 second to enter adjustment mode.
    - Missing button detection.

   Written by Chris Fryer and Jonathan Oxer for www.superhouse.tv
    https://github.com/superhouse/

   Copyright 2019-2020 SuperHouse Automation Pty Ltd www.superhouse.tv
*/
#define VERSION "3.0"
/*--------------------------- Configuration ------------------------------*/
// Configuration should be done in the included file:
#include "config.h"

/*--------------------------- Libraries ----------------------------------*/
// For load cell amplifier:
#include "HX711.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <EEPROM.h>

/*--------------------------- Global Variables ---------------------------*/
// OLED:
uint32_t g_last_screen_update   = 0;

// Rotary encoder:
int32_t  g_old_encoder_position = -999;
uint32_t g_last_activity_time   = 0;
uint8_t  g_last_button_state    = 0;
uint8_t  g_current_button_state = 0;
uint32_t g_last_debounce_time   = 0;
uint16_t g_debounce_interval    = 500;

// Non-volatile memory:
uint32_t g_eeprom_address       = 0;

/* 227 (Pocophone) = 920000 */
// Load cell:
int32_t g_trigger_level         = DEFAULT_TRIGGER_LEVEL;
int32_t g_pressure_level        = 0;
uint8_t g_adjust_mode           = false;
uint8_t g_button_pressed        = false;
int32_t g_zero_pressure_offset  = 0;

// General:
uint32_t g_beep_start_time      = 0;
uint8_t  g_beep_stretch         = false;

/*--------------------------- Function Signatures ---------------------------*/
void checkIfThresholdReached();
void readPressureLevel();
int  getScaledLoadSensorValue();
void checkUiTimeout();
void readRotaryEncoder();
void updateOledDisplay();
void readLoadCell();
void checkButton();
void checkTareButton();
void checkBeepStretchSwitch();
void tareCellReading();

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
  Serial.begin(SERIAL_BAUD_RATE);
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  Serial.print("SuperButton starting up, v");
  Serial.println(VERSION);

  pinMode(ENCODER_SWITCH,   INPUT_PULLUP);
  pinMode(ENCODER_PIN_A,    INPUT_PULLUP);
  pinMode(ENCODER_PIN_B,    INPUT_PULLUP);

  pinMode(TARE_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(BEEP_STRETCH_PIN, INPUT_PULLUP);

  pinMode(LED_PIN,    OUTPUT);
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
  display.println("Starting");
  display.print(" button");
  display.display();
  for (uint8_t i = 0; i < 3; i++)
  {
    delay(300);
    display.print(".");
    display.display();
  }

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(400); // Load cell requires ~15ms to settle. Allow much longer for safety.

  // Do an automatic "tare" operation at startup
  tareCellReading();

  // We need a way to detect if the button isn't plugged in. Because the HX711
  // is internal, it always gives a reading. With no button it reads -1, but
  // that could be a legitimate tare offset so we cant just check for that.

  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("  Ready");
  display.display();
  delay(1000);
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
  checkBeepStretchSwitch();
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
      g_beep_start_time = millis();
    }
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(OUTPUT_PIN, HIGH);
    g_last_activity_time = millis();
  } else {
    if (ENABLE_SERIAL_DEBUGGING)
    {
      Serial.println();
    }
    if ((g_beep_stretch == true && (millis() - g_last_activity_time > g_beep_stretch))
        || g_beep_stretch == false)
    {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(OUTPUT_PIN, LOW);
    }
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
    new_encoder_position = (int)encoder.read() / 4; // Divide by 4 so we only get 1 step per detent
    if (new_encoder_position > g_old_encoder_position)
    {
      g_trigger_level++;
      g_last_activity_time = millis();
      // Prevent the trigger level going above 100%
      if (g_trigger_level > 100)
      {
        g_trigger_level = 100;
      }
      Serial.print("Trigger level: ");
      Serial.print(g_trigger_level);
      Serial.println("%");
      g_old_encoder_position = new_encoder_position;
    }
    if (new_encoder_position < g_old_encoder_position)
    {
      g_trigger_level--;
      g_last_activity_time = millis();
      // Prevent the trigger level going below 1%
      if (g_trigger_level < 1)
      {
        g_trigger_level = 1;
      }
      Serial.print("Trigger level: ");
      Serial.print(g_trigger_level);
      Serial.println("%");
      g_old_encoder_position = new_encoder_position;
    }
  } else {
    // Even if we don't want to read the encoder, consume the value
    // from it so that a queued change doesn't get applied the moment
    // we enter adjustment mode. We also use this to un-blank the
    // display if the knob is twiddled.
    new_encoder_position = (int)encoder.read() / 4; // Divide by 4 so we only get 1 step per detent
    if (new_encoder_position != g_old_encoder_position)
    {
      g_old_encoder_position = new_encoder_position;
      g_last_activity_time = millis();
      //Serial.println(new_encoder_position);
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
  g_last_button_state = g_current_button_state;
  g_current_button_state = digitalRead(ENCODER_SWITCH);

  // Transition from unpressed to pressed
  if (LOW == g_current_button_state && HIGH == g_last_button_state)
  {
    g_last_activity_time = millis();  // Used to un-blank display

    // If we haven't waited long enough then ignore this press
    if (millis() - g_last_debounce_time <= g_debounce_interval) {
      return;
    }

    Serial.print("Button pressed.");
    g_last_debounce_time = millis();

    if (true == g_adjust_mode)
    {
      g_adjust_mode = false;
      Serial.println(" Locking screen.");
      EEPROM.put(g_eeprom_address, g_trigger_level);
      Serial.print("Saving trigger level: ");
      Serial.print(g_trigger_level);
      Serial.println("%");
    } else {
      g_adjust_mode = true;
      Serial.println(" Unlocking screen.");
    }
  }
}

/**
  Check the position of the beep stretch switch
*/
void checkBeepStretchSwitch()
{
  uint8_t switch_position;
  switch_position = digitalRead(BEEP_STRETCH_PIN);

  if (LOW == switch_position)
  {
    g_beep_stretch = true;
  } else {
    g_beep_stretch = false;
  }
}

/**
  Check if the tare button is pressed
*/
void checkTareButton()
{
  uint8_t switch_position;
  switch_position = digitalRead(TARE_BUTTON_PIN);

  if (LOW == switch_position)
  {
    // Button is pressed, do the tare!
    tareCellReading();
  }
}

/**
  Reset the zero position of the load cell
*/
void tareCellReading()
{
  // Read the load cell to set the zero offset
  g_zero_pressure_offset = -1 * getScaledLoadCellValue();
#if ENABLE_SERIAL_DEBUGGING
  Serial.print("Tare offset: ");
  Serial.println(g_zero_pressure_offset, DEC);
#endif
}
