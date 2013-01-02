hexbright_jrt by Jeff Thieleke

Significant changes from hexbright_factory (default factory code):
  * Incorporated accelerometer, 15 minute no-motion power off, and 2 second delay->power switch->power off  
     shortcut code from https://github.com/bhimoff/HexBrightFLEX
  * Modified the duty cycle on the blinking mode to be more bicycle friendly
  * Added a last-on memory (including for the blinking light level)
  * Code reformatting and reorganization

  
There are two functionally similar sketches:
  * programs/hexbright_jrt        (uses dhiltonp's hexbright library)
  * programs/hexbright_jrt_nonlib (non-library version)

If you are looking to modify one of these sketches, I would start with the library version
since it is cleaner and builds upon the excellent hexbright library.  If sketch size and
(potentially) free RAM is more important, the non-library version would be a better choice.  






-----------------------------------------------------------------------------------------------
hexbright library README.md from https://github.com/dhiltonp/hexbright
-----------------------------------------------------------------------------------------------



hexbright
=========

This is the easiest way to get started with programming your hexbright.

First, download and install arduino (http://arduino.cc/en/Main/Software) and the CP210x driver (Use a VCP Driver Kit from http://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx).  Most linux kernels come with the driver pre-built.

Next, download this folder (git clone or download and extract the zip), open the arduino ide, and click on 'File'->'Preferences' in the menu.  
Set your sketchbook location to the location of this folder (where this README file is found).

Restart arduino.

Now, click on 'Tools'->'Board'->'Hexbright' as your device type.
With your hexbright unplugged, go to 'Tools'->'Serial Port' and look at the options.
Now plug in your hexbright and go to 'Tools'->'Serial Port'.  Select the new option.  On linux, there may be a delay of over a minute before the device appears.
Underneath the 'Sketch' and 'Tools' menu options, there is an up arrow (to open a program).  Click on it, go to 'programs', and select a program.

'temperature_calibration' is one of the simplest programs you could write.

'functional' is a basic example of how a program might have multiple modes.

'down_test' contains an example of using the accelerometer.

libraries/hexbright/hexbright.h has a list of all available methods in the api, and is fairly well commented.

Be aware that this library is a work in progress.  In particular, the accelerometer api may change, and it is not yet optimized.

Enjoy!
