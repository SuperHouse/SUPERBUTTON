/* ----------------- General Config -------------------------------- */
#define ENABLE_HAPTIC_FEEDBACK   true  // Vibrate on activation
#define DEFAULT_TRIGGER_LEVEL      50  // Replaced by value from EEPROM
#define ADJUSTMENT_MODE_TIMEOUT    20  // Seconds
#define SCREEN_TIMEOUT_INTERVAL    30  // Seconds
#define SCREEN_UPDATE_INTERVAL    200  // 200ms = 5 updates per second
#define ENABLE_SERIAL_DEBUGGING false  // true / false

/* ----------------- Hardware-specific Config ---------------------- */
/* Rotary encoder */
#define ENCODER_PIN_A       1
#define ENCODER_PIN_B       0
#define ENCODER_SWITCH     A0

/* Load cell */
#define LOADCELL_SCK_PIN    4
#define LOADCELL_DOUT_PIN   5

/* Output */
#define OUTPUT_PIN          6
#define HAPTIC_PIN         12
#define LED_PIN            13
