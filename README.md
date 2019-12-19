"SuperButton" Assistive Technology Button
=========================================

Arduino firmware for the SuperButton Assistive Technology button.

Provides a simple dry-contact output assistive-technology
button using a load cell, so that the force required to activate
the button can be adjusted to suit the needs of the user.

The activation pressure can be adjusted by clicking the rotary
encoder and then rotating the encoder. Clicking the button again
applies the new setting and saves it in EEPROM. When it hasn't
been clicked into adjustment mode, the rotary encoder is ignored
so that it can't be accidentally bumped to a different setting.

More information: https://www.superhouse.tv/sb

To do:
 * Time out adjustment mode if no change within X seconds.
 * Require button press for 1 second to enter adjustment mode.
 * Debounce button.

Contributers:
 * Chris Fryer <chris.fryer78@gmail.com>
 * Jonathan Oxer <jon@oxer.com.au>

Copyright 2019 SuperHouse Automation Pty Ltd www.superhouse.tv
