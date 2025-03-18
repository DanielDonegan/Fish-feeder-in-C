//
// Created by Neal Snooke on 12/07/2024.
// Version 1.1
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <_cygwin.h>

#include "fish.h"
//#include "fish.c"

/**
 * this function does the users processing
 * replace this function with your own processing code for the assignment
 * Note: when this function ends the program (and GUI will close)
 * This is a test of some of the main fish feeder emulator functions
 * @return
 */
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define CLOSE_MENUS (-1)
#define MAIN_MENU_ID 0
#define OPTIONS_MENU_ID 1
#define FEED_MENU_ID 2
#define UTILITY_MENU_ID 3
#define DATE_SET_MENU_ID 4
#define OPERATING_MODE_MENU_ID 5
#define SPEED_MENU_ID 6
#define DISPLAY_TIMES_MENU_ID 7
#define SET_CLOCK_MENU_ID 8

#define FILE_TO_WRITE_TO "C:\\Users\\danjo\\CLionProjects\\2024-2025_fish_C\\FeedSchedule.txt"

#define COOLDOWN_TIME 60
#define TIMEOUT_TIME 60
#define ROTATION_SPEED 55 //The value stands for the milliseconds that we will be waiting between each motor step. At 55 the speed is at a maximum as the feeder will take 20s to feed
#define LARGEST_ROTATION_SPEED 100

#define NUMBER_OF_DATE_SET_ITEMS 3
#define NUMBER_OF_CLOCK_SET_ITEMS 6

#define LINE_SIZE 80 // size of the line buffer

#define stringify2(x) stringify(x)

//This is a repeat of code in the fish.c file as for some reason the main.c file can't access it from there; same problem ClockStruct
typedef struct timeStruct {
    int hour;
    int minute;
    int numRots;
} FeedTime;

typedef struct clockStruct {
    int second;
    int minute;
    int hour;
    int day;
    int month;
    int year;
} ClockTime;

/**
 * the function that is the entry point for the fish feeder C program main logic
 * it is called by jniSetup() from main, once the GUI thread has been initialised.
 *
 * this is a test implementation that demonstrates the clock, button and display
 * It should be split into smaller functions.
 */

int setClockMenu(int *currentTSI, int *currentTSV, ClockTime *clockTimeValsToSave, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    //Here you can manually set the number of variables you would like to use to be able to specify the new clock time
    int numValsInClockSetter = 6;

    displayText(0, 0, "Set clock menu:", 1);

    displayDefaultClockValues(SCREEN_WIDTH, SCREEN_HEIGHT, numValsInClockSetter, clockTimeValsToSave);
    displayNumberPicker(SCREEN_WIDTH, SCREEN_HEIGHT, currentTSI, currentTSV);

    char *result = buttonState();

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*currentTSV)++;
    }

    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;

        //The arduous checking the value of i is unfortunately necessary due to the nature of a struct, only being able to access the variables inside
        // of it through direct reference to the variable's identifier
        if (*currentTSI == 0) {
            clockTimeValsToSave->second = *currentTSV;
        }else if (*currentTSI == 1) {
            clockTimeValsToSave->minute = *currentTSV;
        }else if (*currentTSI == 2) {
            clockTimeValsToSave->hour = *currentTSV;
        }else if (*currentTSI == 3) {
            clockTimeValsToSave->day = *currentTSV;
        }else if (*currentTSI == 4) {
            clockTimeValsToSave->month = *currentTSV;
        }else if (*currentTSI == 5) {
            clockTimeValsToSave->year = *currentTSV;
        }

        (*currentTSI)++;
        *currentTSV = 0;
    }

    if (*currentTSI >= NUMBER_OF_CLOCK_SET_ITEMS) {
        //Setting the clock to what the user requested
        clockSet(clockTimeValsToSave->second, clockTimeValsToSave->minute, clockTimeValsToSave->hour, clockTimeValsToSave->day, clockTimeValsToSave->month, clockTimeValsToSave->year);

        //Reset all the clock setting values
        *currentTSI = 0;
        clockTimeValsToSave->second = 0;
        clockTimeValsToSave->minute = 0;
        clockTimeValsToSave->hour = 0;
        clockTimeValsToSave->day = 0;
        clockTimeValsToSave->month = 0;
        clockTimeValsToSave->year = 0;

        return MAIN_MENU_ID;
    }

    return SET_CLOCK_MENU_ID;
}

int displayTimesMenu(char **timesListPtr, int *rangeIndex, int size, int *scrollOffset, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    displayText(0, 0, "Feed times:", 1);

    char *result = buttonState();

    displayScroller(timesListPtr, rangeIndex, SCREEN_WIDTH, SCREEN_HEIGHT, size, scrollOffset);

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*rangeIndex)++;

        //5 is the limit for the number of items to be displayed on screen
        if (*rangeIndex >= 5) {
            (*scrollOffset)++;
        }
    }

    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;

        if (timesListPtr != NULL) {
            removeTimeFromFile(timesListPtr[*rangeIndex], FILE_TO_WRITE_TO);
        }

        //To sync what is now saved in the file with the systems copy of the feed schedule we must clear and reload timesListPtr
        getAllDatesFromFileAsString(timesListPtr, FILE_TO_WRITE_TO);

        return MAIN_MENU_ID;
    }

    return DISPLAY_TIMES_MENU_ID;
}

int speedMenu(int *timeOutCounter, int *rotationSpeed) {
    displayClear();
    displayColour("white", "black");

    displayText(0, SCREEN_HEIGHT / 8, "Motor speed:", 1);

    //There will only ever be one number to pick in this menu so it will only ever be zero hence all of this trouble to make
    int *zero = malloc(sizeof(int));
    *zero = 0;

    displayNumberPicker(SCREEN_WIDTH, SCREEN_HEIGHT, zero, rotationSpeed);

    free(zero);

    char *result = buttonState();

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*rotationSpeed)++;

        if (*rotationSpeed > LARGEST_ROTATION_SPEED) {
            *rotationSpeed = ROTATION_SPEED;
        }
    }

    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;

        return MAIN_MENU_ID;
    }

    return SPEED_MENU_ID;
}

int utilityMenu(int *rangeIndex, char **utilityPtr, int *scrollOffset, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    //Getting the size of the utilityOptions array
    int modeSize = 3;

    //The main menu and the option menu are not included in the menuOptions char* array

    //Display menu description
    displayText(SCREEN_WIDTH / 16, SCREEN_HEIGHT / 64, "Utility menu", 1);

    displayScroller(utilityPtr, rangeIndex, SCREEN_WIDTH, SCREEN_HEIGHT, modeSize, scrollOffset);

    char *result = buttonState();

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*rangeIndex)++;
    }

    //The user has selected this mode option
    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;

        switch (*rangeIndex) {
            case 0:
                return SPEED_MENU_ID;
            break;
            case 1:
                return DISPLAY_TIMES_MENU_ID;
            break;
            case 2:
                return SET_CLOCK_MENU_ID;
            break;
            default:
                return MAIN_MENU_ID;
            break;
        }
    }

    free(result);

    return UTILITY_MENU_ID;
}

int dateSetMenu(int *currentTSI, int *currentTSV, FeedTime *feedTimeValsToSave, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    //Display menu description
    displayText(SCREEN_WIDTH / 16, SCREEN_HEIGHT / 64, "date set menu", 1);

    displayDefaultDateValues(SCREEN_WIDTH, SCREEN_HEIGHT, NUMBER_OF_DATE_SET_ITEMS, feedTimeValsToSave);

    //Just displays and highlights the number the user is currently trying to transform
    displayNumberPicker(SCREEN_WIDTH, SCREEN_HEIGHT, currentTSI, currentTSV);

    char *result = buttonState();

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*currentTSV)++;
    }

    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;
        if (*currentTSI == 0) {
            feedTimeValsToSave->hour = *currentTSV;
        }else if (*currentTSI == 1) {
            feedTimeValsToSave->minute = *currentTSV;
        }else if (*currentTSI == 2) {
            feedTimeValsToSave->numRots = *currentTSV;
        }

        (*currentTSI)++;
        *currentTSV = 0;
    }

    boundTimeVals(currentTSI, currentTSV);

    free(result);

    //If the user is finished setting their new feed time
    if (*currentTSI >= NUMBER_OF_DATE_SET_ITEMS) {
        parseTimeToFile(feedTimeValsToSave, FILE_TO_WRITE_TO);

        //Reset all the date setting values
        *currentTSI = 0;
        feedTimeValsToSave->hour = 0;
        feedTimeValsToSave->minute = 0;
        feedTimeValsToSave->numRots = 1;

        return MAIN_MENU_ID;
    }

    return DATE_SET_MENU_ID;

}

/**
 *
 * @param currentModePtr
 * @param rangeIndex
 * @param optionPtr
 * @param timeOutCounter
 * @return
 */
int operatingModeMenu(enum ModeOption *currentModePtr, int *rangeIndex, char **optionPtr, int *scrollOffset, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    //Getting the size of the modeOptions array
    int modeSize = 5;

    //Display menu description
    displayText(SCREEN_WIDTH / 16, SCREEN_HEIGHT / 64, "Op mode menu", 1);

    //const char* modeOptions[5] = {"Paused", "Auto", "Feed now", "Skip next", "Back"};
    displayScroller(optionPtr, rangeIndex, SCREEN_WIDTH, SCREEN_HEIGHT, modeSize, scrollOffset);

    char *result = buttonState();

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*rangeIndex)++;

        printf("\n");
        printf("\n");
        printf("%d\n", *rangeIndex);
        printf("\n");
        printf("\n");
    }

    //The user has selected this mode option
    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;
        *currentModePtr = *rangeIndex;

        return MAIN_MENU_ID;
    }

    free(result);

    return OPERATING_MODE_MENU_ID;
}

int feedMenu(enum ModeOption *currentModePtr, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    char* title = malloc(20 * sizeof(char));
    sprintf(title, "%s", "Feedey boy");

    logAdd(GENERAL, "feed menu");
    printf("Long press the button to feed, short press to exit\n");

    displayText(0,0, title, 2);

    displayColour( "black", "white");
    displayText(0,CHAR_HEIGHT*4, "Manual feed", 1);
    displayColour("white", "black"); // JavaFX display. white text on black background
    displayText(0,CHAR_HEIGHT*7, "Exit", 1);

    char *result = buttonState(); // get the button state from the JavaFX application

    if (strcmp(result, "SHORT_PRESS") == 0){
        *timeOutCounter = 0;
        return MAIN_MENU_ID;
    }

    if (strcmp(result, "LONG_PRESS") == 0){
        *timeOutCounter = 0;
        *currentModePtr = FeedNow;
        return MAIN_MENU_ID;
    }

    free(result);

    return FEED_MENU_ID;
}

/**
 * The optionMenu() function creates the option menu. The purpose of the option menu is to be able to access all the other menus,
 * hence them being 'options'.
 */
int optionMenu(int *menuIndex, int *timeOutCounter) {
    displayClear();
    displayColour("white", "black");

    //The main menu and the option menu are not included in the menuOptions char* array
    char *menuOptions[5] = {"Feed", "Utility", "dateSet", "Op mode", "Back"};

    //Instructions for the user to travers the options menu are displayed
    displayText(0, 0, "SHORT press to navigate, LONG to select", 1);

    char *result = buttonState();

    //If the menu index has gone outside the range of the list then loop the value back round to 0
    if (*menuIndex > 4) {
        *menuIndex = 0;
    }

    //Displays the new text to say what option you are currently selecting and removes the text from the previous option
    displayClearArea(SCREEN_WIDTH/4, SCREEN_HEIGHT/4, SCREEN_WIDTH, SCREEN_HEIGHT/4);
    displayText(SCREEN_WIDTH/4, SCREEN_HEIGHT/4, menuOptions[*menuIndex], 2);

    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        (*menuIndex)++;
    }

    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;
        switch (*menuIndex) {
            case 0:
                return FEED_MENU_ID;
            case 1:
                return UTILITY_MENU_ID;
            case 2:
                return DATE_SET_MENU_ID;
            case 3:
                return OPERATING_MODE_MENU_ID;
            case 4:
                return MAIN_MENU_ID;
            default:
                return OPTIONS_MENU_ID;
        }
    }

    free(result);

    return OPTIONS_MENU_ID;
}

/**
 *
 * @param title String of the title for the main menu
 * @param currentMode String of the current operating mode so that it can be displayed on the GUI
 * @param numOfFeeds Pointer to an int value that represents the number of times the feeder has fed the fish while the program has been running
 * @return
 */
int mainMenu(char *title, char *currentMode, int *numOfFeeds, char *nextFeedTime, int *timeOutCounter) {
    // output banner (see fish.h for details)
    displayColour("white", "black");
    displayClear();
    displayTitle(title);

    // display console message
    printf("Long press the button to quit, short press to enter menu\n");

    // display graphic lines around time
    displayColour("white", "black");
    displayLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1);
    displayLine(0, SCREEN_HEIGHT - CHAR_HEIGHT*2-1, SCREEN_WIDTH, SCREEN_HEIGHT - CHAR_HEIGHT*2-1);

    // display the time value
    char time[LINE_SIZE];
    snprintf(time, LINE_SIZE, "%02i-%02i-%02i", clockHour(), clockMinute(), clockSecond());
    displayText(SCREEN_WIDTH/2-4*CHAR_WIDTH, SCREEN_HEIGHT-CHAR_HEIGHT*1.5, time, 1);

    //Display the next scheduled feed time
    displayText(SCREEN_WIDTH/1.5-4*CHAR_WIDTH, SCREEN_HEIGHT-CHAR_HEIGHT*3.5, nextFeedTime, 1);

    //Display the number of feeds since the automatic feeding schedule started
    displayNumberOfFeeds(SCREEN_WIDTH/1.5-4*CHAR_WIDTH, SCREEN_HEIGHT-CHAR_HEIGHT*5, *numOfFeeds, 1);

    //Display the feeder operating mode
    displayText(SCREEN_WIDTH/5-4*CHAR_WIDTH, SCREEN_HEIGHT-CHAR_HEIGHT*3.5, currentMode, 1);

    char *result = buttonState(); // get the button state from the JavaFX application

    //Check the result and close the menu system if the button is short pressed
    if (strcmp(result, "LONG_PRESS") == 0) {
        *timeOutCounter = 0;
        return CLOSE_MENUS;
    }

    // check the result and quit this look if the button is short pressed
    if (strcmp(result, "SHORT_PRESS") == 0) {
        *timeOutCounter = 0;
        return OPTIONS_MENU_ID;
    }

    free(result);

    logAdd(METHOD_ENTRY, "userProcessing() done");

    return MAIN_MENU_ID;
}

void menuSelector() {
    int menuID = 0;

    //Size of the feed schedule file in terms of lines
    int *numLinesInFeedFile = malloc(sizeof(int));
    *numLinesInFeedFile = howManyLinesInFile(FILE_TO_WRITE_TO) + 1;

    //General setup for all menu functions
    //Allocate the string for the title memory and write to it
    char* title = malloc(20 * sizeof(char));
    sprintf(title, "%s", "Fishofeed");

    int *menuIndex = malloc(sizeof(int));
    *menuIndex = 0;

    //Number of feeds there has been while the program has been running
    int *numOfFeeds = malloc(sizeof(int));
    *numOfFeeds = 0;

    //Current operating mode that the fish feeder is in
    enum ModeOption *currentModePtr = malloc(sizeof(enum ModeOption));
    *currentModePtr = Auto;

    int *rangeIndexOp = malloc(sizeof(int));
    *rangeIndexOp = 0;

    //OPTION MODE MENU
    char* modeOptions[5] = {"Paused", "Auto", "Feed now", "Skip next", "Back"};

    //Borrowed from the website geeksforgeeks.org for how to malloc an array of strings
    char **optionPtr = (char**)malloc(5 * sizeof(char*));
    if (optionPtr == NULL) {
        fprintf(stderr, "Memory allocation error\n");
    }

    for (int i = 0; i < 5; i++) {
        optionPtr[i] = (char*)malloc(20 * sizeof(char));

        if (optionPtr[i] == NULL) {
            fprintf(stderr, "Memory allocation error\n");
        }else {
            sprintf(optionPtr[i], "%s", modeOptions[i]);
        }
    }

    //DATE SET DATA:
    FeedTime *feedValuesToSave = malloc(sizeof(FeedTime));
    feedValuesToSave->hour = 0;
    feedValuesToSave->minute = 0;
    feedValuesToSave->numRots = 1;
    int *currentTimeSelectorIndex = malloc(sizeof(int));
    *currentTimeSelectorIndex = 0;
    int *currentTimeSelectorValue = malloc(sizeof(int));
    *currentTimeSelectorValue = 0;

    //CLOCK SET DATA
    ClockTime *clockValuesToSave = malloc(sizeof(ClockTime));
    clockValuesToSave->second = 0;
    clockValuesToSave->minute = 0;
    clockValuesToSave->hour = 0;
    clockValuesToSave->day = 0;
    clockValuesToSave->month = 0;
    clockValuesToSave->year = 0;
    int *currentClockSelectorIndex = malloc(sizeof(int));
    *currentClockSelectorIndex = 0;
    int *currentClockSelectorValue = malloc(sizeof(int));
    *currentClockSelectorValue = 0;

    //UTILITY MODE MENU
    //I've left a lot of room for adding extra utility
    char* utilityOptions[3] = {"Speed control", "Display times", "Set clock"};

    int *rangeIndexUt = malloc(sizeof(int));
    *rangeIndexUt = 0;

    char **utilityPtr = (char**)malloc(5 * sizeof(char*));
    if (utilityPtr == NULL) {
        fprintf(stderr, "Memory allocation error\n");
    }

    for (int i = 0; i < 5; i++) {
        utilityPtr[i] = (char*)malloc(20 * sizeof(char));

        if (utilityPtr[i] == NULL) {
            fprintf(stderr, "Memory allocation error\n");
        }else {
            sprintf(utilityPtr[i], "%s", utilityOptions[i]);
        }
    }

    //DATE VIEWING MENU
    char **timesListPtr = (char**)malloc(*numLinesInFeedFile * sizeof(char*));
    if (timesListPtr == NULL) {
        fprintf(stderr, "Memory allocation error\n");
    }

    for (int i = 0; i < *numLinesInFeedFile; i++) {
        timesListPtr[i] = (char*)malloc(20 * sizeof(char));

        if (timesListPtr[i] == NULL) {
            fprintf(stderr, "Memory allocation error\n");
        }else {
            sprintf(timesListPtr[i], "%s", "1");
            printf("%s\n", timesListPtr[i]);
        }
    }

    getAllDatesFromFileAsString(timesListPtr, FILE_TO_WRITE_TO);

    int *rangeIndexDa = malloc(sizeof(int));
    *rangeIndexDa = 0;

    //Ptr to the next time that the feeder will feed and the setup for that variable in terms of memory allocation and converting to string
    FeedTime *nextTimeToFeed = malloc(sizeof(FeedTime));
    getClosestDateFromFile(nextTimeToFeed, FILE_TO_WRITE_TO);
    char *nextTimeToFeedAsString[20];
    nextFeedTimeToString(nextTimeToFeedAsString, nextTimeToFeed);

    //When a menu function is called it will return an id. If the user did nothing it will just return that menu's id, if they participated in an action which required changing
    // menus then the menu function will return the id of the next menu function to go to. The ids for the menu functions are specified through the pre-processor
    int runningMenus = 1;

    int baseMotorTurn = 360;
    int currentMotorTurn = baseMotorTurn;
    int rotationsLeftToComplete = nextTimeToFeed->numRots - 1;

    int prev_sec = clockSecond(); // allow detection when seconds value has changed
    int prev_min = clockMinute();

    int haveMovedThisMinute = 0;
    int areMoving = 0;

    int *timeOutCounter = malloc(sizeof(int));
    *timeOutCounter = 0;

    //The rotation offset changes if we stop feeding whilst in the middle of the process, rotationOffset will hold the degree of rotation to make sure that the feeder finsishes
    // in the same position that it started
    int *rotationOffset = malloc(sizeof(int));
    *rotationOffset = currentMotorTurn;

    //For the scrolling menus this is the offset that increments everything in the to scroll list by 1
    int *scrollOffset = malloc(sizeof(int));
    *scrollOffset = 0;

    //SPEED MENU
    int *rotationSpeed = malloc(sizeof(int));
    *rotationSpeed = ROTATION_SPEED;

    while (runningMenus) {
        //This condition contains all the operations that require transforming or checking when the seconds increment
        if (clockSecond() != prev_sec) {
            //We don't need to worry about having a condition for the mode option being Paused as when the system is paused it will not be doing anything anyway
            if (*currentModePtr == Auto) {
                if (clockMinute() == nextTimeToFeed->minute && clockHour() == nextTimeToFeed->hour && !haveMovedThisMinute) {
                    areMoving = 1;
                }
            }else if (*currentModePtr == Skip) {
                //We skip the next feed, and then we go back into automatic feeding
                getNextClosestDateFromFile(nextTimeToFeed, FILE_TO_WRITE_TO);
                rotationsLeftToComplete = nextTimeToFeed->numRots - 1;
                nextFeedTimeToString(nextTimeToFeedAsString, nextTimeToFeed);
                *currentModePtr = Auto;
            }else if (*currentModePtr == FeedNow) {
                //Feeds instantly with one rotation
                areMoving = 1;
                rotationsLeftToComplete = 0;
                *currentModePtr = Auto;
            }

            (*timeOutCounter)++;
            prev_sec = clockSecond();
        }

        if (prev_min != clockMinute()) {
            haveMovedThisMinute = 0;
            prev_min = clockMinute();
        }

        if (areMoving) {
            motorStep();
            motorDisplay(SCREEN_WIDTH / 20, (SCREEN_HEIGHT / 6) - CHAR_HEIGHT, currentMotorTurn);

            //When currentMotorTurn == 0 we know we have come to the end of the motor cycle and so reset the relative variables for the next feed time
            if (currentMotorTurn == 0 && rotationsLeftToComplete == 0) {
                haveMovedThisMinute = 1;
                areMoving = 0;
                currentMotorTurn = 360;
                (*numOfFeeds)++;

                nextFeedTimeToString(nextTimeToFeedAsString, nextTimeToFeed);

                getClosestDateFromFile(nextTimeToFeed, FILE_TO_WRITE_TO);
                rotationsLeftToComplete = nextTimeToFeed->numRots - 1;
            }else if (currentMotorTurn == 0 && rotationsLeftToComplete > 0) {
                currentMotorTurn = baseMotorTurn;
                rotationsLeftToComplete--;
            }else {
                currentMotorTurn--;
            }
        }else {

            //Jump to the menu the user is currently using
            switch (menuID) {
                case MAIN_MENU_ID:
                    menuID = mainMenu(title, intToModeOptionConverter(*currentModePtr), numOfFeeds, nextTimeToFeedAsString, timeOutCounter);
                break;
                case OPTIONS_MENU_ID:
                    menuID = optionMenu(menuIndex, timeOutCounter);
                break;
                case FEED_MENU_ID:
                    menuID = feedMenu(currentModePtr, timeOutCounter);
                break;
                case UTILITY_MENU_ID:
                    menuID = utilityMenu(rangeIndexUt, utilityPtr, scrollOffset, timeOutCounter);
                break;
                case DATE_SET_MENU_ID:
                    menuID = dateSetMenu(currentTimeSelectorIndex, currentTimeSelectorValue, feedValuesToSave, timeOutCounter);
                break;
                case OPERATING_MODE_MENU_ID:
                    menuID = operatingModeMenu(currentModePtr, rangeIndexOp, optionPtr, scrollOffset, timeOutCounter);
                break;
                case SPEED_MENU_ID:
                    menuID = speedMenu(timeOutCounter, rotationSpeed);
                break;
                case DISPLAY_TIMES_MENU_ID:
                    menuID = displayTimesMenu(timesListPtr, rangeIndexDa, *numLinesInFeedFile, scrollOffset, timeOutCounter);
                break;
                case SET_CLOCK_MENU_ID:
                    menuID = setClockMenu(currentClockSelectorIndex, currentClockSelectorValue, clockValuesToSave, timeOutCounter);
                break;
            }
        }

        //If the user hasn't clicked the button in a while turn the screen off to maintain screen life
        if (*timeOutCounter >= TIMEOUT_TIME) {
            displayClear();
        }

        // check for the button state every 0.2 second
        if (areMoving) {
            msleep(*rotationSpeed);
        }else {
            msleep(100L);
        }

        //When menuId is -1 that means the user has selected to exit the menu
        if (menuID == -1) {
            runningMenus = 0;
        }
    }

    //Free all the memory that we allocated for variables earlier
    for (int i = 0; i < 5; i++) {
        free(optionPtr[i]);
    }

    for (int i = 0; i < *numLinesInFeedFile; i++) {
        free(timesListPtr[i]);
    }

    for (int i = 0; i < 2; i++) {
        free(utilityPtr[i]);
    }

    //Free all the memory we allocated at the start of the function
    free(title);
    free(menuIndex);
    free(numOfFeeds);
    free(currentModePtr);
    free(numLinesInFeedFile);
    free(rotationSpeed);
    free(rotationOffset);
    free(rangeIndexOp);
    free(rangeIndexUt);
    free(rangeIndexDa);

    free(currentTimeSelectorIndex);
    free(currentTimeSelectorValue);
    free(feedValuesToSave);

    free(optionPtr);
    free(timesListPtr);
    free(utilityPtr);
}

void userProcessing() {

    // add a message to the log METHOD_ENTRY category (see fish.h for details)
    logAdd(METHOD_ENTRY, "userProcessing()");

    // display an information message on the GUI debug log display area (see fish.h)
    infoMessage("Welcome to the fish feeder test program");

    // set the real time clock to 13:00:00 on the 25th May 1968
    clockSet(55, 30, 14, 25, 05, 1968); // set the time to 12:30:00
    printf("Clock thinks the year is %d\n", clockYear());

    foodFill(50); // fill the food container Note: maximum 50%

    //Sets up and runs the main menu
    menuSelector();
}

/**
 * main entry point for the Fish Feeder Emulator program
 * @return
 */
int main() {
    // turn on all console logging for the Java and fish feeder JNI C code. Shouldn't need this
    //logAddInfo(GENERAL | METHOD_ENTRY | JNI_MESSAGES | JFX_MESSAGES | THREAD_NAME | THREAD_ID | STACK_INFO);
    logAddInfo( GENERAL | JFX_MESSAGES | THREAD_NAME | THREAD_ID );

    //logRemoveInfo(METHOD_ENTRY); // don't show these debug messages in the gui

    // add the GUI debug window showing messages received
    logAddInfo( GUI_INFO_DEBUG);

    // add a log entry for entry to this method
    logAdd(METHOD_ENTRY, "main(). test of Fish Feed Hardware Emulator using a JavaFX GUI and jni");

    // start the JVM and set up the JNI environment
    // this will result in the userProcessing() function being called to run the C part of the program
    if (jniSetup() != 0) return 1;

    // hand over this thread to Java FX to create the fish hardware emulator GUI.
    // FX will terminate the application when the main window is closed
    // or the C application calls the FishFeederEmulator.exit() function.
    javaFx();

    return(EXIT_SUCCESS);
}