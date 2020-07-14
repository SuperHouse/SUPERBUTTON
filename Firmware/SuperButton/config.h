/* ----------------- General Config -------------------------------- */
#define DEFAULT_TRIGGER_LEVEL       50   // % of full pressure. Replaced by value from EEPROM
#define ADJUSTMENT_MODE_TIMEOUT     20   // Seconds
#define SCREEN_TIMEOUT_INTERVAL     30   // Seconds
#define SCREEN_UPDATE_INTERVAL     200   // 200ms = 5 updates per second

#define BEEP_STRETCH_PERIOD       2000   // ms

/* Serial */
#define ENABLE_SERIAL_DEBUGGING  true   // true / false
#define SERIAL_BAUD_RATE          9600   // Speed for USB serial console

/* ----------------- Hardware-specific Config ---------------------- */
/* Rotary encoder */
#define ENCODER_PIN_A       1            // Input pullup applied
#define ENCODER_PIN_B       0            // Input pullup applied
#define ENCODER_SWITCH     A0            // Input pullup applied
#define DEBOUNCE_PERIOD   500            // ms

/* Inputs */
#define TARE_BUTTON_PIN    A1
#define BEEP_STRETCH_PIN    7

/* Load cell */
#define LOADCELL_SCK_PIN    4            // HX711 Sck
#define LOADCELL_DOUT_PIN   5            // HX711 Dout

/* Output */
#define OUTPUT_PIN          6            // Controls relay
#define LED_PIN            13            // Controls internal LED
