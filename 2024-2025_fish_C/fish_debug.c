/**
 * Author Neal Snooke 2024-07-05
 * Version 0.81
 *
 * This is a mock up of the functions intended to call the JavaFX GUI
 * This version uses the console to log calls and produces the display
 * as an SVG file that can be viewed in the browser. This it is single threaded and
 * does not produce SIGSEV signals (as java does as part of it memory management causing
 * false 'crash' signals in most debuggers used by CLion)
 *
 * After each call to a display function the file will be updated to reflect
 * the current contents of the display. The user must refresh the broswer page to
 * view the updatesd display.
 *
 * All output from the calls to GUI functions will be prefixed with "GUI:"
 */

// this is an API - we expect functions that are not used in the current project
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "fish.h"

// string buffer size
#define LINE_SIZE 200

// it is possible to output various levels of debug info from the Fish GUI Emulator Java and C code
// the following constants are used to select what to output to the console log.
// the debug level is a single integer, with information selected by bitwise OR of the following constants
const int THREAD_NAME = 1 << 0; // append the thread name to all log messages
const int THREAD_ID = 1 << 1; // append the thread id to all log messages
const int METHOD_ENTRY = 1 << 2; // method entry and exit messages
const int JNI_MESSAGES = 1 << 3; // Java Native Interface messages
const int JFX_MESSAGES = 1 << 4; // JavaFX processing messages
const int GENERAL = 1 << 5;
const int STACK_INFO = 1 << 6; // stack trace information
const int GUI_INFO_DEBUG = 1<<7;

// the character font for the display
const unsigned char BASIC_FONT[] = {
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
        0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
        0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
        0x18, 0x3C, 0x7E, 0x3C, 0x18,
        0x1C, 0x57, 0x7D, 0x57, 0x1C,
        0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
        0x00, 0x18, 0x3C, 0x18, 0x00,
        0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
        0x00, 0x18, 0x24, 0x18, 0x00,
        0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
        0x30, 0x48, 0x3A, 0x06, 0x0E,
        0x26, 0x29, 0x79, 0x29, 0x26,
        0x40, 0x7F, 0x05, 0x05, 0x07,
        0x40, 0x7F, 0x05, 0x25, 0x3F,
        0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
        0x7F, 0x3E, 0x1C, 0x1C, 0x08,
        0x08, 0x1C, 0x1C, 0x3E, 0x7F,
        0x14, 0x22, 0x7F, 0x22, 0x14,
        0x5F, 0x5F, 0x00, 0x5F, 0x5F,
        0x06, 0x09, 0x7F, 0x01, 0x7F,
        0x00, 0x66, 0x89, 0x95, 0x6A,
        0x60, 0x60, 0x60, 0x60, 0x60,
        0x94, 0xA2, 0xFF, 0xA2, 0x94,
        0x08, 0x04, 0x7E, 0x04, 0x08,
        0x10, 0x20, 0x7E, 0x20, 0x10,
        0x08, 0x08, 0x2A, 0x1C, 0x08,
        0x08, 0x1C, 0x2A, 0x08, 0x08,
        0x1E, 0x10, 0x10, 0x10, 0x10,
        0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
        0x30, 0x38, 0x3E, 0x38, 0x30,
        0x06, 0x0E, 0x3E, 0x0E, 0x06,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x5F, 0x00, 0x00,
        0x00, 0x07, 0x00, 0x07, 0x00,
        0x14, 0x7F, 0x14, 0x7F, 0x14,
        0x24, 0x2A, 0x7F, 0x2A, 0x12,
        0x23, 0x13, 0x08, 0x64, 0x62,
        0x36, 0x49, 0x56, 0x20, 0x50,
        0x00, 0x08, 0x07, 0x03, 0x00,
        0x00, 0x1C, 0x22, 0x41, 0x00,
        0x00, 0x41, 0x22, 0x1C, 0x00,
        0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
        0x08, 0x08, 0x3E, 0x08, 0x08,
        0x00, 0x80, 0x70, 0x30, 0x00,
        0x08, 0x08, 0x08, 0x08, 0x08,
        0x00, 0x00, 0x60, 0x60, 0x00,
        0x20, 0x10, 0x08, 0x04, 0x02,
        0x3E, 0x51, 0x49, 0x45, 0x3E,
        0x00, 0x42, 0x7F, 0x40, 0x00,
        0x72, 0x49, 0x49, 0x49, 0x46,
        0x21, 0x41, 0x49, 0x4D, 0x33,
        0x18, 0x14, 0x12, 0x7F, 0x10,
        0x27, 0x45, 0x45, 0x45, 0x39,
        0x3C, 0x4A, 0x49, 0x49, 0x31,
        0x41, 0x21, 0x11, 0x09, 0x07,
        0x36, 0x49, 0x49, 0x49, 0x36,
        0x46, 0x49, 0x49, 0x29, 0x1E,
        0x00, 0x00, 0x14, 0x00, 0x00,
        0x00, 0x40, 0x34, 0x00, 0x00,
        0x00, 0x08, 0x14, 0x22, 0x41,
        0x14, 0x14, 0x14, 0x14, 0x14,
        0x00, 0x41, 0x22, 0x14, 0x08,
        0x02, 0x01, 0x59, 0x09, 0x06,
        0x3E, 0x41, 0x5D, 0x59, 0x4E,
        0x7C, 0x12, 0x11, 0x12, 0x7C,
        0x7F, 0x49, 0x49, 0x49, 0x36,
        0x3E, 0x41, 0x41, 0x41, 0x22,
        0x7F, 0x41, 0x41, 0x41, 0x3E,
        0x7F, 0x49, 0x49, 0x49, 0x41,
        0x7F, 0x09, 0x09, 0x09, 0x01,
        0x3E, 0x41, 0x41, 0x51, 0x73,
        0x7F, 0x08, 0x08, 0x08, 0x7F,
        0x00, 0x41, 0x7F, 0x41, 0x00,
        0x20, 0x40, 0x41, 0x3F, 0x01,
        0x7F, 0x08, 0x14, 0x22, 0x41,
        0x7F, 0x40, 0x40, 0x40, 0x40,
        0x7F, 0x02, 0x1C, 0x02, 0x7F,
        0x7F, 0x04, 0x08, 0x10, 0x7F,
        0x3E, 0x41, 0x41, 0x41, 0x3E,
        0x7F, 0x09, 0x09, 0x09, 0x06,
        0x3E, 0x41, 0x51, 0x21, 0x5E,
        0x7F, 0x09, 0x19, 0x29, 0x46,
        0x26, 0x49, 0x49, 0x49, 0x32,
        0x03, 0x01, 0x7F, 0x01, 0x03,
        0x3F, 0x40, 0x40, 0x40, 0x3F,
        0x1F, 0x20, 0x40, 0x20, 0x1F,
        0x3F, 0x40, 0x38, 0x40, 0x3F,
        0x63, 0x14, 0x08, 0x14, 0x63,
        0x03, 0x04, 0x78, 0x04, 0x03,
        0x61, 0x59, 0x49, 0x4D, 0x43,
        0x00, 0x7F, 0x41, 0x41, 0x41,
        0x02, 0x04, 0x08, 0x10, 0x20,
        0x00, 0x41, 0x41, 0x41, 0x7F,
        0x04, 0x02, 0x01, 0x02, 0x04,
        0x40, 0x40, 0x40, 0x40, 0x40,
        0x00, 0x03, 0x07, 0x08, 0x00,
        0x20, 0x54, 0x54, 0x78, 0x40,
        0x7F, 0x28, 0x44, 0x44, 0x38,
        0x38, 0x44, 0x44, 0x44, 0x28,
        0x38, 0x44, 0x44, 0x28, 0x7F,
        0x38, 0x54, 0x54, 0x54, 0x18,
        0x00, 0x08, 0x7E, 0x09, 0x02,
        0x18, 0xA4, 0xA4, 0x9C, 0x78,
        0x7F, 0x08, 0x04, 0x04, 0x78,
        0x00, 0x44, 0x7D, 0x40, 0x00,
        0x20, 0x40, 0x40, 0x3D, 0x00,
        0x7F, 0x10, 0x28, 0x44, 0x00,
        0x00, 0x41, 0x7F, 0x40, 0x00,
        0x7C, 0x04, 0x78, 0x04, 0x78,
        0x7C, 0x08, 0x04, 0x04, 0x78,
        0x38, 0x44, 0x44, 0x44, 0x38,
        0xFC, 0x18, 0x24, 0x24, 0x18,
        0x18, 0x24, 0x24, 0x18, 0xFC,
        0x7C, 0x08, 0x04, 0x04, 0x08,
        0x48, 0x54, 0x54, 0x54, 0x24,
        0x04, 0x04, 0x3F, 0x44, 0x24,
        0x3C, 0x40, 0x40, 0x20, 0x7C,
        0x1C, 0x20, 0x40, 0x20, 0x1C,
        0x3C, 0x40, 0x30, 0x40, 0x3C,
        0x44, 0x28, 0x10, 0x28, 0x44,
        0x4C, 0x90, 0x90, 0x90, 0x7C,
        0x44, 0x64, 0x54, 0x4C, 0x44,
        0x00, 0x08, 0x36, 0x41, 0x00,
        0x00, 0x00, 0x77, 0x00, 0x00,
        0x00, 0x41, 0x36, 0x08, 0x00,
        0x02, 0x01, 0x02, 0x04, 0x02,
        0x3C, 0x26, 0x23, 0x26, 0x3C,
        0x1E, 0xA1, 0xA1, 0x61, 0x12,
        0x3A, 0x40, 0x40, 0x20, 0x7A,
        0x38, 0x54, 0x54, 0x55, 0x59,
        0x21, 0x55, 0x55, 0x79, 0x41,
        0x21, 0x54, 0x54, 0x78, 0x41,
        0x21, 0x55, 0x54, 0x78, 0x40,
        0x20, 0x54, 0x55, 0x79, 0x40,
        0x0C, 0x1E, 0x52, 0x72, 0x12,
        0x39, 0x55, 0x55, 0x55, 0x59,
        0x39, 0x54, 0x54, 0x54, 0x59,
        0x39, 0x55, 0x54, 0x54, 0x58,
        0x00, 0x00, 0x45, 0x7C, 0x41,
        0x00, 0x02, 0x45, 0x7D, 0x42,
        0x00, 0x01, 0x45, 0x7C, 0x40,
        0xF0, 0x29, 0x24, 0x29, 0xF0,
        0xF0, 0x28, 0x25, 0x28, 0xF0,
        0x7C, 0x54, 0x55, 0x45, 0x00,
        0x20, 0x54, 0x54, 0x7C, 0x54,
        0x7C, 0x0A, 0x09, 0x7F, 0x49,
        0x32, 0x49, 0x49, 0x49, 0x32,
        0x32, 0x48, 0x48, 0x48, 0x32,
        0x32, 0x4A, 0x48, 0x48, 0x30,
        0x3A, 0x41, 0x41, 0x21, 0x7A,
        0x3A, 0x42, 0x40, 0x20, 0x78,
        0x00, 0x9D, 0xA0, 0xA0, 0x7D,
        0x39, 0x44, 0x44, 0x44, 0x39,
        0x3D, 0x40, 0x40, 0x40, 0x3D,
        0x3C, 0x24, 0xFF, 0x24, 0x24,
        0x48, 0x7E, 0x49, 0x43, 0x66,
        0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
        0xFF, 0x09, 0x29, 0xF6, 0x20,
        0xC0, 0x88, 0x7E, 0x09, 0x03,
        0x20, 0x54, 0x54, 0x79, 0x41,
        0x00, 0x00, 0x44, 0x7D, 0x41,
        0x30, 0x48, 0x48, 0x4A, 0x32,
        0x38, 0x40, 0x40, 0x22, 0x7A,
        0x00, 0x7A, 0x0A, 0x0A, 0x72,
        0x7D, 0x0D, 0x19, 0x31, 0x7D,
        0x26, 0x29, 0x29, 0x2F, 0x28,
        0x26, 0x29, 0x29, 0x29, 0x26,
        0x30, 0x48, 0x4D, 0x40, 0x20,
        0x38, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x38,
        0x2F, 0x10, 0xC8, 0xAC, 0xBA,
        0x2F, 0x10, 0x28, 0x34, 0xFA,
        0x00, 0x00, 0x7B, 0x00, 0x00,
        0x08, 0x14, 0x2A, 0x14, 0x22,
        0x22, 0x14, 0x2A, 0x14, 0x08,
        0xAA, 0x00, 0x55, 0x00, 0xAA,
        0xAA, 0x55, 0xAA, 0x55, 0xAA,
        0x00, 0x00, 0x00, 0xFF, 0x00,
        0x10, 0x10, 0x10, 0xFF, 0x00,
        0x14, 0x14, 0x14, 0xFF, 0x00,
        0x10, 0x10, 0xFF, 0x00, 0xFF,
        0x10, 0x10, 0xF0, 0x10, 0xF0,
        0x14, 0x14, 0x14, 0xFC, 0x00,
        0x14, 0x14, 0xF7, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0x00, 0xFF,
        0x14, 0x14, 0xF4, 0x04, 0xFC,
        0x14, 0x14, 0x17, 0x10, 0x1F,
        0x10, 0x10, 0x1F, 0x10, 0x1F,
        0x14, 0x14, 0x14, 0x1F, 0x00,
        0x10, 0x10, 0x10, 0xF0, 0x00,
        0x00, 0x00, 0x00, 0x1F, 0x10,
        0x10, 0x10, 0x10, 0x1F, 0x10,
        0x10, 0x10, 0x10, 0xF0, 0x10,
        0x00, 0x00, 0x00, 0xFF, 0x10,
        0x10, 0x10, 0x10, 0x10, 0x10,
        0x10, 0x10, 0x10, 0xFF, 0x10,
        0x00, 0x00, 0x00, 0xFF, 0x14,
        0x00, 0x00, 0xFF, 0x00, 0xFF,
        0x00, 0x00, 0x1F, 0x10, 0x17,
        0x00, 0x00, 0xFC, 0x04, 0xF4,
        0x14, 0x14, 0x17, 0x10, 0x17,
        0x14, 0x14, 0xF4, 0x04, 0xF4,
        0x00, 0x00, 0xFF, 0x00, 0xF7,
        0x14, 0x14, 0x14, 0x14, 0x14,
        0x14, 0x14, 0xF7, 0x00, 0xF7,
        0x14, 0x14, 0x14, 0x17, 0x14,
        0x10, 0x10, 0x1F, 0x10, 0x1F,
        0x14, 0x14, 0x14, 0xF4, 0x14,
        0x10, 0x10, 0xF0, 0x10, 0xF0,
        0x00, 0x00, 0x1F, 0x10, 0x1F,
        0x00, 0x00, 0x00, 0x1F, 0x14,
        0x00, 0x00, 0x00, 0xFC, 0x14,
        0x00, 0x00, 0xF0, 0x10, 0xF0,
        0x10, 0x10, 0xFF, 0x10, 0xFF,
        0x14, 0x14, 0x14, 0xFF, 0x14,
        0x10, 0x10, 0x10, 0x1F, 0x00,
        0x00, 0x00, 0x00, 0xF0, 0x10,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
        0xFF, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0xFF,
        0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
        0x38, 0x44, 0x44, 0x38, 0x44,
        0x7C, 0x2A, 0x2A, 0x3E, 0x14,
        0x7E, 0x02, 0x02, 0x06, 0x06,
        0x02, 0x7E, 0x02, 0x7E, 0x02,
        0x63, 0x55, 0x49, 0x41, 0x63,
        0x38, 0x44, 0x44, 0x3C, 0x04,
        0x40, 0x7E, 0x20, 0x1E, 0x20,
        0x06, 0x02, 0x7E, 0x02, 0x02,
        0x99, 0xA5, 0xE7, 0xA5, 0x99,
        0x1C, 0x2A, 0x49, 0x2A, 0x1C,
        0x4C, 0x72, 0x01, 0x72, 0x4C,
        0x30, 0x4A, 0x4D, 0x4D, 0x30,
        0x30, 0x48, 0x78, 0x48, 0x30,
        0xBC, 0x62, 0x5A, 0x46, 0x3D,
        0x3E, 0x49, 0x49, 0x49, 0x00,
        0x7E, 0x01, 0x01, 0x01, 0x7E,
        0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
        0x44, 0x44, 0x5F, 0x44, 0x44,
        0x40, 0x51, 0x4A, 0x44, 0x40,
        0x40, 0x44, 0x4A, 0x51, 0x40,
        0x00, 0x00, 0xFF, 0x01, 0x03,
        0xE0, 0x80, 0xFF, 0x00, 0x00,
        0x08, 0x08, 0x6B, 0x6B, 0x08,
        0x36, 0x12, 0x36, 0x24, 0x36,
        0x06, 0x0F, 0x09, 0x0F, 0x06,
        0x00, 0x00, 0x18, 0x18, 0x00,
        0x00, 0x00, 0x10, 0x10, 0x00,
        0x30, 0x40, 0xFF, 0x01, 0x01,
        0x00, 0x1F, 0x01, 0x01, 0x1E,
        0x00, 0x19, 0x1D, 0x17, 0x12,
        0x00, 0x3C, 0x3C, 0x3C, 0x3C,
        0x00, 0x00, 0x00, 0x00, 0x00,
};

// since this is only a mock up of the GUI simulation for the purposes of allowing testing
// without multiple threads and JavaFX/jni calls we make do with some globals for simplicity

int  log_level = 0; // global that stores the current log level setting

// the real time clock
time_t RTC_offset; // the offset for the rtc

FILE * output_file ;
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_SCALE 5
#define COLOUR_MAX_CHARS 25
// with of the border around each pixel
#define SVG_STROKE_WIDTH "0.5"

// the OLED display
char oled[DISPLAY_WIDTH][DISPLAY_HEIGHT][COLOUR_MAX_CHARS+1]; //25 max colour name length
char display_bg[COLOUR_MAX_CHARS+1] = "BLACK";
char display_fg[COLOUR_MAX_CHARS+1] = "WHITE";
#define DISPLAY_SVG_FILENAME "display.svg"

// feeder motor
#define STEP_ANGLE 1
static int motor_steps = 0;
int rotationAngle = 0;
double SCOOP_SIZE = 0.01; // amount of food taken at each full scoop
double MIN_FOOD = 0.01; // prevent problems if no food exists.
double foodVolume = 0.25;

/**
 * sleep for a number of milliseconds (posix sleep() is seconds)
 * @param msec
 * @return 0 if successful, -1 if unsuccessful
 */
int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0) return -1;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    res = nanosleep(&ts, &ts);
    return res;
}

/**
 * for the java FX version this will setup the JNI environment
 * and locates the Java classes and methods required
 * in this simple non GUI version all the function needs to do is call the users main logic function
 * @return
 */
int jniSetup() {
    printf("GUI: jniSetup()\n");
    userProcessing();
    return 0;
}

/**
 * in the real JavaFX application this function will set up the javaFX application
 * since this version does not
 * @return 0 if successful 1 if unsuccessful
 */
int javaFx() {
    printf("GUI: javaFx()\n");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Motor functions
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * send message to the JavaFX application
 * to step the motor
 */
void motorStep() {
    printf("GUI: MOTOR_STEP ");
    //call_j_command(build_args("s", "MOTOR_STEP")); // 1st argument is format specifier
    rotationAngle -= STEP_ANGLE;

    if (rotationAngle < 0) {
        rotationAngle = rotationAngle+360;
    }

    // find angle when scoop emerges from food
    if (rotationAngle < 180 && rotationAngle+STEP_ANGLE >=180) {
        //reduce food (simplified version at 180 degrees)
        foodVolume -= SCOOP_SIZE;
        if (foodVolume < MIN_FOOD) foodVolume = MIN_FOOD;
        printf("GUI: FOOD VOLUME CHANGED %f\n", foodVolume*100);
    }

    printf("Angle %d Food Volume %f \n", rotationAngle, foodVolume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DISPLAY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * create an svg file for the mock up display output file provided
 * @param output_filename
 */
void create_svg_file_header(char * output_filename) {
    output_file = fopen(output_filename, "w");

    // size of graphic
    fprintf(output_file, "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n",
            (DISPLAY_WIDTH+2)*DISPLAY_SCALE, (DISPLAY_HEIGHT+2)*DISPLAY_SCALE);

    // background rectangle/border
    fprintf(output_file,
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:%s;\" />\n",
            0, 0, (DISPLAY_WIDTH+2)*DISPLAY_SCALE, (DISPLAY_HEIGHT+2)*DISPLAY_SCALE, "GREY");
}

/**
 * create the svg footer
 */
// create a function to create an svg file footer and write it to the given file
void create_svg_file_footer() {
    fprintf(output_file, "</svg>\n");
    fclose(output_file);
}

/**
 * draw the display as an svg file
 */
void output_display_svg(){
    for (int col = 0; col<DISPLAY_WIDTH; col++){
        for (int row = 0; row<DISPLAY_HEIGHT; row++){
            fprintf(output_file,
                    "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
                    "style=\"fill:%s;stroke-width:%s;stroke:rgb(0,0,0)\" />\n",
                    (col+1)*DISPLAY_SCALE, (row+1)*DISPLAY_SCALE, DISPLAY_SCALE, DISPLAY_SCALE,
                    oled[col][row], SVG_STROKE_WIDTH );
        }
    }
}

/**
 * output the display to an svg file
 */
void saveDisplay(){
    create_svg_file_header(DISPLAY_SVG_FILENAME);
    output_display_svg();
    create_svg_file_footer();
}

/**
 * clear the display
 */
void displayClear() {
    printf("GUI: CLEAR_DISPLAY\n");
    //call_j_command(build_args("s", "CLEAR_DISPLAY"));
    displayClearArea(0,0,DISPLAY_WIDTH, DISPLAY_HEIGHT);
    saveDisplay();
}

/**
 * clear a specified region of the display
 * @param x top left cartesian position x
 * @param y top left cartesian position x
 * @param w width of area to clear in pixels
 * @param h height of area to clear in pixels
 */
void displayClearArea(int x, int y, int w, int h) {
    printf("GUI: CLEAR_AREA %d %d %d %d\n", x, y, w, h);
    //call_j_command(build_args("sdddd", "CLEAR_AREA", x, y, w, h)); // 1st argument is format specifier
    for (int col = x; col<(x+w); col++){
        for (int row = 0; row<(y+h); row++){
            strncpy(oled[col][row], display_bg, COLOUR_MAX_CHARS);
        }
    }
}

/**
 * set a pixel in the display array to a specified colour
 * @param x pixel position cartesian x
 * @param y pixel position cartesian y
 * @param colour - string that specifies the colour and svg colour will work
 */
void displayColourPixel(int x, int y, char* colour) {
    if (x >= 0 && y >= 0 && x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT) {
        strncpy(oled[x][y], colour, COLOUR_MAX_CHARS);
    }
}

/**
 * send message to the JavaFX application
 * to display a line on the JavaFX application display
 * @param x pixel position start of line cartesian x
 * @param y pixel position start of line cartesian y
 * @param x1 pixel position end of line cartesian x
 * @param y1 pixel position end of line cartesian y
 */
void displayLine(int x0, int y0, int x1, int y1) {
    printf("GUI: LINE %d %d %d %d\n", x0, y0, x1, y1);
    //call_j_command(build_args("sdddd", "LINE", x, y, w, h)); // 1st argument is format specifier

    // bresenham's algorithm for the pixels on a straight line - thx Wikpedia
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    int t;

    if (steep) {
        t = x0; x0 = y0; y0 = t; //swap(x0, y0);
        t = x1; x1 = y1; y1 = t; //swap(x1, y1);
    }

    if (x0 > x1) {
        t = x0; x0=x1; x1=t; //swap(x0, x1);
        t = y0; y0=y1; y1=t; //swap(y0, y1);
    }

    int dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int err = dx / 2;
    int ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            displayColourPixel(y0, x0, display_fg);
        } else {
            displayColourPixel(x0, y0, display_fg);
        }

        err -= dy;

        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }

    saveDisplay();
}

/**
 * send message to the JavaFX application
 * to display a pixel on the JavaFX application display
 * @param x
 * @param y
 */
void displayPixel(int x, int y) {
    printf("GUI: PIXEL %d %d\n", x, y);
    //call_j_command(build_args("sdd", "PIXEL", x, y)); // 1st argument is format specifier
    displayColourPixel(x,y, display_fg);

    saveDisplay();
}

/**
 * draw a single character on the display
 * @param x pixel position x
 * @param y pixel position y
 * @param c the ASCII character to display
 * @param size
 */
void drawChar(int x, int y, char c, int size) {
    // draw the 5 pixel columns
    for (int column=0; column<6; column++ ) {
        // lookup pixels for row of character
        int line = 0;
        if (column != 5) {
            line = BASIC_FONT[((int)c*5)+column];
        }

        char* colour;

        // draw the 8 pixels in row
        for (int row = 0; row<8; row++) {

            colour = ((line & 0x1) == 1) ? display_fg : display_bg;

            if ((strlen(colour)!=0)) { // if the colour isn't empty string
                for (int w = 0; w < size; w++) {
                    for (int h = 0; h < size; h++) {
                        displayColourPixel(x + column*size +w, y + row*size +h, colour);
                    }
                }
            }

            // next pixel
            line >>= 1;
        }

    }
}

/**
 * display text string on the JavaFX application display
 * @param x - x pixel position
 * @param y - x pixel position
 * @param text - the text to display
 * @param size - 1 or 2 are the only two sizes currently supported on the real display
 */
void displayText(int x, int y, char *text, int size) {
    printf("GUI: TEXTXY %d %d %s %d\n", x, y, text, size);
    //call_j_command(build_args("sddsd", "TEXTXY", x, y, text, size)); // 1st argument is format specifier
    for (int i = 0; i<strlen(text); i++){
        drawChar(x+(i*6*size), y, *(text+i), size);
    }

    saveDisplay();
}

/**
 * change the colours of the display
 * the real display is black and white only, but colours are supported in the emulator
 * the colours are in the form of a hex string e.g. #FFFFFF or javas FX colour names
 * https://docs.oracle.com/javase/8/javafx/api/javafx/scene/paint/Color.html
 * an empty string is transparent when used as the background colour
 *
 * @param fg forground colour
 * @param bg background colour
 */
void displayColour(char *fg, char *bg) {
    printf("GUI: MESSAGE %s %s\n", fg, bg);
    strncpy(display_fg, fg, COLOUR_MAX_CHARS);
    strncpy(display_bg, bg, COLOUR_MAX_CHARS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BUTTON FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * send a message to the JavaFX application and get a response
 * @return
 */
char *buttonState() {
    printf("GUI: BUTTON ");

    char c, *t;

    // input button state from user skipping the newlines
    printf("Button Pressed? no, short, long (n,s,l)");
    do {
        c = (char)getchar();
    } while (c=='\n');

    switch (c) {
        case 'S': case 's': t = "SHORT_PRESS"; break;
        case 'L': case 'l': t = "LONG_PRESS"; break;
        default : t = "NO_PRESS"; break;
    }

    // create the heap allocated result
    char * result = malloc (strlen(t)+1);
    strcpy(result, t);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CLOCK FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * send message to the JavaFX application
 * to set the real time clock (RTC) time
 * @param sec
 * @param min
 * @param hour
 * @param day
 * @param month
 * @param year
 */
void clockSet(int sec, int min, int hour, int day, int month, int year) {
    printf("GUI: SET_RTC %d %d %d %d %d %d\n", sec, min, hour, day, month, year);

    struct tm tm_rtc = {.tm_sec = sec, .tm_min = min, .tm_hour = hour,
            .tm_mday = day, .tm_mon = month-1, .tm_year = (year-1900), .tm_isdst = 1};

    RTC_offset = time(NULL) - mktime(&tm_rtc);
}

/**
 * Either set the clock or fetch the current clock offset (from real time).
 * This is not the way to set the time - do that with clockSet()
 * This is to allow the emulator to have the 'battery backup' function of the clock.
 *   ie the clock only needs to be set once, even if the system is not used for months.
 * If the value 0 is provided the current offset is returned.
 * Calling the function at any time in the future with that same offset will cause the emulator clock
 * to to be behave as though it had been running continuously.
 * e.g if it was an hour slow, setting that same warm start value obtained at another time
 * will mean it is still an hour slow relative to the current real time.
 * @param offset - a value obtained previously by this function for a clock.
 * @return
 */
long long clockWarmStart(long long offset) {
    printf("GUI: RTC_WARM_START %lld\n", offset);

    if (offset != 0) {
        RTC_offset = offset;
    }

    return RTC_offset;
}

/**
 * @return seconds
 */
int clockSecond() {
    printf("GUI: clockSecond()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_sec;
}

/**
 *
 * @return minutes
 */
int clockMinute() {
    printf("GUI: clockMinute()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_min;
}

/**
 * @return hours
 */
int clockHour() {
    printf("GUI: clockHour()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_hour;
}

/**
 * @return day of month
 */
int clockDay() {
    printf("GUI: clockDay()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_mday;
}

/**
 * @return month (1-12)
 */
int clockMonth() {
    printf("GUI: clockMonth()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_mon;
}

/**
 * @return year
 */
int clockYear() {
    printf("GUI: clockYear()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_year+1900;
}

/**
 * @return 0-6 (Sunday-> Saturday)
 */
int clockDayOfWeek() {
    printf("GUI: clockDayOfWeek()\n");
    time_t a = time(NULL) - RTC_offset;
    return localtime(&a)->tm_wday;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MECHANICS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * fill the food container to a percentage and
 * send message to the JavaFX application
 * @param foodLevel
 */
void foodFill(int foodLevel) {
    printf("GUI: SET FOOD %d%%\n", foodLevel);
    foodVolume = (double)foodLevel/100;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// LOG FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * send an information message to the JavaFX application info area
 * @param text
 */
void infoMessage(char *text) {
    printf("GUI: INFO MESSAGE %s\n", text);
}

/**
 * add a log level to the selected output
 * @param i
 */
void logAddInfo(int i){
    log_level = log_level | i;
}

/**
 * remove a log level from the selected output
 * @param i
 */
void logRemoveInfo(int i){
    log_level = log_level & (~i);
}

/**
 * add to the log output
 * @param l - the log level for the message. Only the log levels currently selected by log_level will be output
 * @param message
 */
void logAdd(int l, char *message) {
    char sb[LINE_SIZE] = "C:    ";
    size_t position = 6;

    // if the message matched a current logging level
    if ((l & log_level) > 0) {

        // indent general messages
        if ((l & (GENERAL | JNI_MESSAGES | JFX_MESSAGES)) > 0) {
            position += snprintf(sb+position, (LINE_SIZE-position), "   ");
        }

        // add the message and print to console
        position += snprintf(sb+position, (LINE_SIZE-position), "%s", message);
        printf("%s\n", sb);
        fflush(stdout);
    }
}