/* ----------------- General Config -------------------------------- */
#define ENABLE_HAPTIC_FEEDBACK    true   // Vibrate on activation
#define DEFAULT_TRIGGER_LEVEL       50   // % of full pressure. Replaced by value from EEPROM
#define ADJUSTMENT_MODE_TIMEOUT     20   // Seconds
#define SCREEN_TIMEOUT_INTERVAL     30   // Seconds
#define SCREEN_UPDATE_INTERVAL     200   // 200ms = 5 updates per second

#define PULSE_STRETCHING         false   // true/false. Keeps output on for minimum period.
#define PULSE_STRETCH_PERIOD      1000   // ms

/* Serial */
#define ENABLE_SERIAL_DEBUGGING  false   // true / false
#define SERIAL_BAUD_RATE          9600   // Speed for USB serial console

/* ----------------- Hardware-specific Config ---------------------- */
/* Rotary encoder */
#define ENCODER_PIN_A       1            // Input pullup applied
#define ENCODER_PIN_B       0            // Input pullup applied
#define ENCODER_SWITCH     A0            // Input pullup applied
#define DEBOUNCE_PERIOD   500            // ms

/* Load cell */
#define LOADCELL_SCK_PIN    4            // HX711 Sck
#define LOADCELL_DOUT_PIN   5            // HX711 Dout

/* Output */
#define OUTPUT_PIN          6            // Controls relay
#define HAPTIC_PIN         12            // Controls vibration motor
#define LED_PIN            13            // Controls internal LED
