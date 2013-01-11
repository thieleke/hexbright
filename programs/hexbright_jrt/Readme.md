hexbright_jrt by Jeff Thieleke
==============================

Operation: 
----------
To switch between light levels, click the power button in succession to go from
low to medium to high.  If you pause more than 2 seconds between clicks, the 
HexBright will turn off with the next click.

To activate the flashing mode, start with the HexBright turned off and hold down
the power button for a moment.  The light level for flashing mode is set based on
the last non-flashing light level (medium or high only).

If the HexBright is motionless for more than 15 minutes, it will automatically shut
off to conserve battery power.



Notes:
------

The sketch is based on [hexbright_factory](https://github.com/hexbright/samples/blob/master/hexbright_factory.ino) (default code shipped with the HexBright
flashlight), with the following major changes:

* Built using the hexbright library - <https://github.com/dhiltonp/hexbright>

* Includes a 15 minute no-motion power off and the ability to turn the flashlight
  off without cycling through all of the light levels, inspired by 
  <https://github.com/bhimoff/HexBrightFLEX>

* The flashing rate for blinking mode is more bicycle-friendly - 75% on / 25% off

* The light level (low/medium/high) is remembered and restored when the HexBright
  is next turned on.
