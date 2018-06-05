/*
 *  US_Distance.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Shows the distances measured by a HC-SR04 ultrasonic sensor
 *  Can be used as a parking assistance.
 *  This is an example for using a fullscreen GUI.

 *  Copyright (C) 2015, 2016  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay.
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "ArduinoUtils.h"

#include "BlueDisplay.h"

#define VERSION_EXAMPLE "1.1"

// Change this if you have reprogrammed the hc05 module for higher baud rate
//#define HC_05_BAUD_RATE BAUD_9600
#define HC_05_BAUD_RATE BAUD_115200

int ECHO_IN_PIN = 2;
int TRIGGER_OUT_PIN = 3;
#define TONE_PIN 4
#define MEASUREMENT_INTERVAL_MS 50

#define DISTANCE_TIMEOUT_CM 350  // cm timeout for US reading

int sActualDisplayWidth;
int sActualDisplayHeight;
int sCaptionTextSize;
int sCaptionStartX;
int sValueStartY;
int sCount;

/*
 * The Start Stop button
 */
BDButton TouchButtonStartStop;
BDButton TouchButtonOffset;
bool doTone = true;
int sOffset = 0;

BDSlider SliderShowDistance;

char sStringBuffer[100];
/*
 * Change doTone flag as well as color and caption of the button.
 */
void doStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
    doTone = aValue;
    if (!aValue) {
        // Stop tone
        BlueDisplay1.playTone(TONE_SILENCE);
    }
}

/*
 * Handler for number receive event - set delay to value
 */
void doSetOffset(float aValue) {
// clip value
    if (aValue > 20) {
        aValue = 20;
    } else if (aValue < -20) {
        aValue = -20;
    }
    sOffset = aValue;
}
/*
 * Request delay value as number
 */
void doGetOffset(BDButton * aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.getNumberWithShortPrompt(&doSetOffset, "Offset distance [cm]");
}

/*
 * Handle change from landscape to portrait
 */
void handleConnectAndReorientation(void) {
//    tone(TONE_PIN, 1000, 50);
    // manage positions according to actual display size
    sActualDisplayWidth = BlueDisplay1.getMaxDisplayWidth();
    sActualDisplayHeight = BlueDisplay1.getMaxDisplayHeight();
    if (sActualDisplayWidth < sActualDisplayHeight) {
        // Portrait -> change to landscape 3/2 format
        sActualDisplayHeight = (sActualDisplayWidth / 3) * 2;
    }
    sCaptionTextSize = sActualDisplayHeight / 4;
    // Position Caption at middle of screen
    sCaptionStartX = (sActualDisplayWidth - (getTextWidth(sCaptionTextSize) * strlen("Distance"))) / 2;

    sprintf(sStringBuffer, "sActualDisplayWidth=%d", sActualDisplayWidth);
    BlueDisplay1.debugMessage(sStringBuffer);

    if (sCaptionStartX < 0) {
        sCaptionStartX = 0;
    }

    sValueStartY = getTextAscend(sCaptionTextSize * 2) + sCaptionTextSize + sCaptionTextSize / 4;
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_TOUCH_BASIC_DISABLE, sActualDisplayWidth, sActualDisplayHeight);

    SliderShowDistance.init(0, sCaptionTextSize * 3, sCaptionTextSize / 4, sActualDisplayWidth, 199, 0, COLOR_BLUE,
    COLOR_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderShowDistance.setScaleFactor(200.0 / sActualDisplayWidth);

    // Initialize button position, size, colors etc.
    TouchButtonStartStop.init(0, BUTTON_HEIGHT_5_DYN_LINE_5, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_5_DYN, COLOR_BLUE, "Start Tone",
            sCaptionTextSize / 3, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doTone, &doStartStop);
    TouchButtonStartStop.setCaptionForValueTrue("Stop Tone");

    TouchButtonOffset.init(BUTTON_WIDTH_3_DYN_POS_3, BUTTON_HEIGHT_5_DYN_LINE_5, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_5_DYN, COLOR_RED,
            "Offset", sCaptionTextSize / 3, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGetOffset);
}

/*
 * Function used as callback handler for redraw event
 */
void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR_BLUE);
    BlueDisplay1.drawText(sCaptionStartX, sCaptionTextSize, "Distance", sCaptionTextSize, COLOR_WHITE, COLOR_BLUE);
    SliderShowDistance.drawSlider();
    TouchButtonStartStop.drawButton();
    TouchButtonOffset.drawButton();
}

void setup(void) {
    pinMode(TRIGGER_OUT_PIN, OUTPUT);
    pinMode(TONE_PIN, OUTPUT);
    pinMode(ECHO_IN_PIN, INPUT);

#ifndef USE_STANDARD_SERIAL
    initSimpleSerial(HC_05_BAUD_RATE, false);
#else
    Serial.begin(HC_05_BAUD_RATE);
#endif

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&handleConnectAndReorientation, &drawGui);

    /*
     * on double tone, we received max canvas size. Otherwise no connection is available.
     */
    if (BlueDisplay1.mConnectionEstablished) {
        tone(TONE_PIN, 3000, 50);
        delay(100);
    } else {
        Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from  " __DATE__));
    }
    tone(TONE_PIN, 3000, 50);
    delay(100);
}

int sCentimeterOld = 50;
bool sToneIsOff = true;

void loop(void) {
    // Timeout of 20000L is 3.4 meter
    getUSDistanceAsCentiMeterWithCentimeterTimeout(DISTANCE_TIMEOUT_CM);
//    startUSDistanceAsCentiMeterWithCentimeterTimeoutNonBlocking(DISTANCE_TIMEOUT_CM);
//    while (!isUSDistanceMeasureFinished()) {
//    }

    if (!BlueDisplay1.mConnectionEstablished) {
        Serial.print("cm=");
        if (sUSDistanceCentimeter >= DISTANCE_TIMEOUT_CM) {
            Serial.println("timeout");
        } else {
            Serial.print(sUSDistanceCentimeter);
            Serial.print(" micros=");
            Serial.println(sUSPulseMicros);
        }
    }

    if (sUSDistanceCentimeter >= DISTANCE_TIMEOUT_CM) {
        // timeout happened
        tone(TONE_PIN, 1000, 50);
        delay(100);
        tone(TONE_PIN, 2000, 50);
        delay((100 - MEASUREMENT_INTERVAL_MS) - 20);

    } else {
        if (doTone && sUSDistanceCentimeter < 100) {
            /*
             * local feedback for distances < 100 cm
             */
            tone(TONE_PIN, sUSDistanceCentimeter * 32, MEASUREMENT_INTERVAL_MS + 20);
        }
        sUSDistanceCentimeter -= sOffset;
        if (sUSDistanceCentimeter != sCentimeterOld) {
            if (BlueDisplay1.mConnectionEstablished) {
                uint16_t tCmXPosition = BlueDisplay1.drawUnsignedByte(getTextWidth(sCaptionTextSize * 2), sValueStartY,
                        sUSDistanceCentimeter, sCaptionTextSize * 2, COLOR_YELLOW,
                        COLOR_BLUE);
                BlueDisplay1.drawText(tCmXPosition, sValueStartY, "cm", sCaptionTextSize, COLOR_WHITE, COLOR_BLUE);
                SliderShowDistance.setActualValueAndDrawBar(sUSDistanceCentimeter);
            }
            if (sUSDistanceCentimeter >= 40 || !doTone) {
                /*
                 * Silence here
                 */
                if (!sToneIsOff) {
                    sToneIsOff = true;
                    if (BlueDisplay1.mConnectionEstablished) {
                        // Stop tone
                        BlueDisplay1.playTone(TONE_SILENCE);
                    }
                }
            } else {
                sToneIsOff = false;
                /*
                 * Switch tones only if range changes
                 */
                if (BlueDisplay1.mConnectionEstablished) {
                    if (sUSDistanceCentimeter < 40 && sUSDistanceCentimeter > 30
                            && (sCentimeterOld >= 40 || sCentimeterOld <= 30)) {
                        BlueDisplay1.playTone(22);
                    } else if (sUSDistanceCentimeter <= 30 && sUSDistanceCentimeter > 20
                            && (sCentimeterOld >= 30 || sCentimeterOld <= 20)) {
                        BlueDisplay1.playTone(17);
                    } else if (sUSDistanceCentimeter <= 20 && sUSDistanceCentimeter > 10
                            && (sCentimeterOld > 20 || sCentimeterOld <= 10)) {
                        BlueDisplay1.playTone(18);
                    } else if (sUSDistanceCentimeter <= 10 && sUSDistanceCentimeter > 3
                            && (sCentimeterOld > 10 || sCentimeterOld <= 3)) {
                        BlueDisplay1.playTone(16);
                    } else if (sUSDistanceCentimeter <= 3 && sCentimeterOld > 3) {
                        BlueDisplay1.playTone(36);
                    }
                }
            }
        }
    }
    checkAndHandleEvents();
    sCentimeterOld = sUSDistanceCentimeter;
    delay(MEASUREMENT_INTERVAL_MS); // < 200
}