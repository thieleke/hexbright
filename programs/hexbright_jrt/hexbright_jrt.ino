/*

 Factory firmware for HexBright FLEX
 v2.4  Dec 6, 2012
 Modifications from Jeff Thieleke:
    * Incorporated accelerometer, 30 minute timeout mode, and the 2 second power off shortcut code from https://github.com/bhimoff/HexBrightFLEX
    * Modified the duty cycle on the blinking mode to be more bicycle friendly
    * Added a last-on memory (including blinking brightness level)
    * Code reformatting and reorganization
 */

#define DEBUG DEBUG_ON

#include <hexbright.h>
#include <EEPROM.h>
#include <Wire.h>

#define DPIN_RLED_SW            2
#define DPIN_DRV_EN             10

// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_BLINKING           4
#define MODE_BLINKING_PREVIEW   5
// EEPROM
#define EEPROM_LAST_ON_MODE		0

// Defaults
const int defaultMode = MODE_HIGH;
const int buttonTimeoutToOffMilliseconds = 2000;
const int noAccelShutoffMilliseconds = 30 * 60 * 1000;

// State
byte mode = 0;
byte lastOnMode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;
int lastChargeState = 0;
unsigned long lastModeTime = 0;
unsigned long oneSecondLoopTime = 0;
unsigned long lastAccelTime = 0;
unsigned long noAccelShutoffTime = 0;
unsigned long poweringOffTime = 0;

hexbright hb;

void setup()
{
    // We just powered on!  That means either we got plugged
    // into USB, or the user is pressing the power button.
    hb = hexbright();
    hb.init_hardware();

    mode = MODE_OFF;
    lastOnMode = 0;
    btnTime = 0;
    btnDown = false;
    lastChargeState = 0;
    lastModeTime = 0;
    oneSecondLoopTime = 0;
    lastAccelTime = 0;
    noAccelShutoffTime = 0;

    lastOnMode = EEPROM.read(EEPROM_LAST_ON_MODE);
    if (lastOnMode < MODE_LOW || lastOnMode > MODE_HIGH)
    {
        lastOnMode = MODE_MED;
    }

    resetAccelTimeout();

    Serial.println("Powered up!");
}

void loop()
{
    const unsigned long time = millis();

    if (poweringOffTime > 0 && time > poweringOffTime)
    {
        powerOff();
        return;
    }

    // read values, adjust lights, etc.
    hb.update();

    switch (hb.get_charge_state())
    {
    case CHARGING:
        if (hb.get_led_state(GLED) == LED_OFF)
            hb.set_led(GLED, 200, 200);
        break;
    case CHARGED:
        hb.set_led(GLED, 10);
        break;
    case BATTERY:
        // led auto offs, don't need to turn it off...
        break;
    };

    oneSecondLoop();
	
    switch (mode)
    {
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
        digitalWrite(DPIN_DRV_EN, (time % 600) < 450);
        break;
    }

    // Check for mode changes
    byte newMode = mode;
    int button_time_held = hb.button_held();
    boolean button_released = hb.button_released();

    switch (mode)
    {
    case MODE_OFF:
        if (button_released)
        {
            if (lastModeTime == 0 || time - lastModeTime > 1000)
            {
                Serial.print("lastOnMode = ");
                Serial.println(lastOnMode);
                newMode = lastOnMode;
                lastModeTime = time;
            }
            else
            {
                newMode = MODE_LOW;
            }
            break;
        }
        if (button_time_held > 500)
        {
            newMode = MODE_BLINKING_PREVIEW;
            break;
        }
        break;
    case MODE_LOW:
        if (button_released)
        {
            if (time - lastModeTime > buttonTimeoutToOffMilliseconds)
            {
                newMode = MODE_OFF;
            }
            else
            {
                newMode = MODE_MED;
            }
        }
        break;
    case MODE_MED:
        if (button_released)
        {
            if (time - lastModeTime > buttonTimeoutToOffMilliseconds)
            {
                newMode = MODE_OFF;
            }
            else
            {
                newMode = MODE_HIGH;
            }
        }
        break;
    case MODE_HIGH:
        if (button_released)
        {
            newMode = MODE_OFF;
        }
        break;
    case MODE_BLINKING_PREVIEW:
        // This mode exists just to ignore this button release.
        if (button_released)
        {
            newMode = MODE_BLINKING;
        }
        break;
    case MODE_BLINKING:
        if (button_released)
        {
            if (time - lastModeTime > 2000)
            {
                newMode = MODE_OFF;
            }
        }
        break;
    }

    // Do the mode transitions
    if (newMode != mode)
    {
        lastModeTime = time;

        switch (newMode)
        {
        case MODE_OFF:
            setLightOff();
            break;
        case MODE_LOW:
            setLightLow();
            break;
        case MODE_MED:
            setLightMed();
            break;
        case MODE_HIGH:
            setLightHigh();
            break;
        case MODE_BLINKING:
        case MODE_BLINKING_PREVIEW:
            setLightBlinking();
            break;
        }
    }
}

void oneSecondLoop()
{
    const unsigned long time = millis();

    if (time - oneSecondLoopTime < 1000)
        return;

    oneSecondLoopTime = time;

    // Periodically pull down the button's pin, since
    // in certain hardware revisions it can float.
    //pinMode(DPIN_RLED_SW, OUTPUT);
    //pinMode(DPIN_RLED_SW, INPUT);

    // Check the accelerometer and shut off the light if there hasn't been any recent movement
    checkAccel();
    if (time > noAccelShutoffTime && mode != MODE_OFF)
    {
        Serial.print("No motion in ");
        Serial.print((time - lastAccelTime) / 1000);
        Serial.println(" seconds - shutting off");
        setLightOff();
    }
}

void checkAccel()
{
    // XXX - disable the accelerometer timeout for now...hb.moved() doesn't seem to work at the moment
    resetAccelTimeout();
}

void powerOff()
{
    // Don't unnecessarily update the EEPROM
    byte eepromLastOnMode = EEPROM.read(EEPROM_LAST_ON_MODE);
    if (eepromLastOnMode != lastOnMode)
    {
        EEPROM.write(EEPROM_LAST_ON_MODE, lastOnMode);
    }

    hb.shutdown();
}

void setLightOff()
{
    setLight(MODE_OFF);
}

void setLightLow()
{
    setLight(MODE_LOW);
}

void setLightMed()
{
    setLight(MODE_MED);
}

void setLightHigh()
{
    setLight(MODE_HIGH);
}

void setLightBlinking()
{
    setLight(MODE_BLINKING);
}

void setLight(byte lightMode)
{
    switch (lightMode)
    {
    case MODE_OFF:
        Serial.println("Mode = off");
        hb.set_light(CURRENT_LEVEL, 0, NOW);
        poweringOffTime = millis() + (buttonTimeoutToOffMilliseconds * 3);
        break;
    case MODE_LOW:
        Serial.println("Mode = low");
        hb.set_light(CURRENT_LEVEL, 200, NOW);
        lastOnMode = lightMode;
        poweringOffTime = 0;
        break;
    case MODE_MED:
        Serial.println("Mode = medium");
        hb.set_light(CURRENT_LEVEL, MAX_LOW_LEVEL, NOW);
        lastOnMode = lightMode;
        poweringOffTime = 0;
        break;
    case MODE_HIGH:
        Serial.println("Mode = high");
        hb.set_light(CURRENT_LEVEL, MAX_LEVEL, NOW);
        lastOnMode = lightMode;
        poweringOffTime = 0;
        break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
        Serial.print("Mode = blinking, brightness mode = ");
        Serial.println(lastOnMode);
        setLight(lastOnMode);
        poweringOffTime = 0;
        break;
    }

    mode = lightMode;
    if (mode == MODE_BLINKING_PREVIEW)
        mode = MODE_BLINKING;

    resetAccelTimeout();
}

void resetAccelTimeout()
{
    const unsigned long time = millis();
    lastAccelTime = time;
    noAccelShutoffTime = time + noAccelShutoffMilliseconds;
}



