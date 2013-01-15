/*
 *  hexbright_jrt by Jeff Thieleke
 */

#include <hexbright.h>
#include <EEPROM.h>
#include <Wire.h>

// Settings
#define NO_MOTION_SHUTOFF_MINUTES  15
#define ACCELEROMETER_SENSITIVITY  30               // (1/10th Gs of acceleration)
#define LONG_HOLD                  1000             // Milliseconds
#define BUTTON_CYCLE_TIMEOUT       LONG_HOLD + 500  // Milliseconds

// Pin assignments
#define DPIN_RLED_SW   2
#define DPIN_GLED      5
#define DPIN_PGOOD     7
#define DPIN_PWR       8
#define DPIN_DRV_MODE  9
#define DPIN_DRV_EN    10
#define DPIN_ACC_INT   3
#define APIN_TEMP      0
#define APIN_CHARGE    3

// Modes
#define MODE_OFF       0
#define MODE_LOW       1
#define MODE_MED       2
#define MODE_HIGH      3
#define MODE_BLINKING  4

// EEPROM
#define EEPROM_LAST_ON_MODE_ADDR  0

// State
byte mode = MODE_OFF;
byte lastOnMode = MODE_OFF;
unsigned long lastModeTime = 0;
unsigned long oneSecondLoopTime = 0;
unsigned long lastAccelTime = 0;
unsigned long noAccelShutoffTime = 0;
unsigned long poweringOffTime = 0;

hexbright hb;


void setup()
{
    hb = hexbright();
    hb.init_hardware();

    mode = MODE_OFF;
    lastOnMode = 0;
    lastModeTime = 0;
    oneSecondLoopTime = 0;
    lastAccelTime = 0;
    noAccelShutoffTime = 0;
    poweringOffTime = 0;

    //Serial.begin(9600);

    lastOnMode = EEPROM.read(EEPROM_LAST_ON_MODE_ADDR);
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

    if (poweringOffTime > 0 && (long)(time - poweringOffTime) >= 0)
    {
        powerOff();
        poweringOffTime = 0;
        return;
    }

    // read values, adjust lights, etc.
    hb.update();

    // Blinking mode control
    switch (mode)
    {
        case MODE_BLINKING:
            const int level = (time % 600) < 450 ? getLightLevel(lastOnMode) : 0;
            if (hb.get_light_level() != level)
                hb.set_light(level, level, NOW);
            break;
    }

    // Check for power switch changes
    checkModeChange();

    // Housekeeping tasks
    checkAccel();
    checkChargeState();
    oneSecondLoop();
}

void checkModeChange()
{
    const unsigned long time = millis();
    const BOOL button_pressed = hb.button_pressed();
    const BOOL button_released = hb.button_just_released();
    const int button_pressed_time = hb.button_pressed_time();
    if ((button_pressed && button_pressed_time < LONG_HOLD) || !button_released)
        return;

    byte newMode = mode;

    switch (mode)
    {
        case MODE_OFF:
            if (button_pressed_time > LONG_HOLD)
            {
                newMode = MODE_BLINKING;
                break;
            }

            if (lastModeTime == 0 || time - lastModeTime > 1000)
            {
                // This is a fresh start - restore the last powered on light mode
                Serial.print("Powering up - lastOnMode = ");
                Serial.println(lastOnMode);
                newMode = lastOnMode;
                lastModeTime = time;
            }
            else
            {
                // Continue cycling through the available modes
                newMode = MODE_LOW;
            }
            break;

        case MODE_LOW:
        case MODE_MED:
        case MODE_HIGH:
            if (button_pressed_time > LONG_HOLD)
            {
                newMode = MODE_BLINKING;
                break;
            }

            if (time - lastModeTime > BUTTON_CYCLE_TIMEOUT)
            {
                newMode = MODE_OFF;
            }
            else
            {
                newMode++;
                if (newMode > MODE_HIGH)
                {
                    newMode = MODE_OFF;
                }
            }
            break;

        case MODE_BLINKING:
            if (button_pressed_time > LONG_HOLD)
            {
                newMode = lastOnMode;
            }
            else
            {
                newMode = MODE_OFF;
            }
            break;
    }

    if (newMode != mode)
    {
        lastModeTime = time;
        mode = newMode;

        switch (mode)
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
    pinMode(DPIN_RLED_SW, OUTPUT);
    pinMode(DPIN_RLED_SW, INPUT);

    if (noAccelShutoffTime > 0 && mode > MODE_OFF)
    {
        Serial.print("Time = ");
        Serial.print(time);
        Serial.print(", No notion shut off time = ");
        Serial.println(noAccelShutoffTime);

        if ((long)(time - noAccelShutoffTime) >= 0)
        {
            Serial.print("No motion in ");
            Serial.print((time - lastAccelTime) / 1000);
            Serial.println(" seconds - shutting off");
            setLightOff();
            noAccelShutoffTime = 0;
        }
    }

    Serial.print("RAM available: ");
    Serial.print(hb.freeRam());
    Serial.println("/1024 bytes");
}

void checkAccel()
{
#ifdef ACCELEROMETER
    if (hb.moved(ACCELEROMETER_SENSITIVITY) == true && mode > MODE_OFF)
    {
        resetAccelTimeout();
    }
#else
    noAccelShutoffTime = 0;
#endif
}

void powerOff()
{
    Serial.println("Powering off");
    poweringOffTime = 0;
    noAccelShutoffTime = 0;

    // Don't unnecessarily update the EEPROM
    byte eepromLastOnMode = EEPROM.read(EEPROM_LAST_ON_MODE_ADDR);
    if (eepromLastOnMode != lastOnMode)
    {
        EEPROM.write(EEPROM_LAST_ON_MODE_ADDR, lastOnMode);
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
            hb.set_light(0, 0, NOW);
            poweringOffTime = millis() + (BUTTON_CYCLE_TIMEOUT * 3);
            mode = MODE_OFF;
            return;
        case MODE_LOW:
        case MODE_MED:
        case MODE_HIGH:
            lastOnMode = lightMode;
            break;
        case MODE_BLINKING:
            setLight(lastOnMode);
            break;
    }

	mode = lightMode;
    const int level = getLightLevel(lastOnMode);
    hb.set_light(level, level, NOW);
    poweringOffTime = 0;
    resetAccelTimeout();
}

int getLightLevel(byte m)
{
    switch (m)
    {
        case MODE_OFF:
            return 0;
        case MODE_LOW:
            return 200;
        case MODE_MED:
            return MAX_LOW_LEVEL;
        case MODE_HIGH:
            return MAX_LEVEL;
        default:
            return MAX_LOW_LEVEL;
    }
}

void resetAccelTimeout()
{
    const unsigned long time = millis();
    const unsigned long noAccelShutoffMilliseconds = NO_MOTION_SHUTOFF_MINUTES * 60000;

    lastAccelTime = time;
    noAccelShutoffTime = time + noAccelShutoffMilliseconds;
    Serial.print("noAccelShutoffTime = ");
    Serial.println(noAccelShutoffTime);
}

void checkChargeState()
{
    switch (hb.get_definite_charge_state())
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
}
