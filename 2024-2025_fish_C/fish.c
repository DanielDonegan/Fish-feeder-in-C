/**
 * Author Neal Snooke 2024-07-05
 * Version 0.81
 *
 * This is a simple example of how to call a JavaFX application from C code using JNI.
 * Notes that the JavaFX application is in a module called fishFeederGUI.
 * The JNIEnv is thread specific.  Can't use JNIEnv from another thread.
 * The JavaVM is global and can be used in any thread.
 * The JavaFX application is started in the main thread and then the C processing thread is started.
 * The C processing thread sends messages to the JavaFX application using the
 * FishFeederEmulator.command() and FishFeederEmulator.message() methods.
 * These method then queues events for the JavaFX thread to process.
 * The Java does not currently (need to) call the C code (it is possible to do this using JNI).
 * The C code is expected to poll the Java (emulated gui) GUI code for values or state as required.
 * In practice for the fish feeder this is only:
 *   the state of the button
 *   the time from the real time clock (RTC) chip.
 *
 * The Java application must be in a sub folder of the C project called fishFeederGUI.
 * and must be be compiled into a bespoke java runtime that links the java FX jmods files for the required platform
 * (the build.sh build file will do all of this - a build.bat file is the Windows version).
 * To test the standalone Java application in the FishFeederGUI folder use:
 * ./customjre/bin/java --module fishFeederGUI/fishgui.FishFeederEmulator
 */

// useful information
// https://nachtimwald.com/2017/06/17/calling-java-from-c/
// https://www.codeproject.com/Articles/993067/Calling-Java-from-Cplusplus-with-JNI
// https://nachtimwald.com/2017/06/06/wrapping-a-c-library-in-java/
// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#NewObject
// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html
// jni interface
// https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html
// https://www3.ntu.edu.sg/home/ehchua/programming/java/JavaNativeInterface.html
// to find method signatures for a class use jdk tool: javap -s -p FishFeederEmulator
// jni Programmer’s Guide and Specification:
// https://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.200/files/staff/domaschka/misc/jni_programmers_guide_spec.pdf

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <jni.h>

#include <string.h>
#include <time.h>
#include <pthread.h>

#include "fish.h"

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

int  log_level = 0; // global that stores the current log level setting

JavaVM *vm;
JNIEnv *env_fx; // jvm environment for JavaFX thread
JNIEnv *env_c; // jvm environment for C processing thread

// the java classes, methods and signatures type names we need
// Note: to find method signature strings for a java class use jdk tool: javap -s -p FishFeederEmulator.class
// the class files are generated in the cls folder by the build script
// note signature package hierarchy but with slashes
char const *const jmethod_name_main = "main";
char const *const jmethod_sig_main = "([Ljava/lang/String;)V";
char const *const jmethod_name_command = "command";
char const *const jmethod_sig_command = "([Ljava/lang/String;)V";
char const *const jmethod_name_message = "message";
char const *const jmethod_sig_message = "([Ljava/lang/String;)Ljava/lang/String;";
char const *const jmethod_name_emulator_exit = "exit";
char const *const jmethod_sig_emulator_exit = "([Ljava/lang/String;)V";
char const *const jmethod_name_isGUIReady = "isGUIReady";
char const *const jmethod_sig_isGUIReady = "()Z";
char const *const jmethod_name_platform_exit = "exit";
char const *const jmethod_sig_platform_exit = "()V";
char const *const jclass_name_String = "java/lang/String";
char const *const jclass_name_FishFeederEmulator = "fishgui/FishFeederEmulator";
char const *const jclass_name_Platform = "javafx/application/Platform";

// java classes and methods that we will need to access
// to make it the lookup as simple as possible these are all static
jclass jclass_FishFeederEmulator = NULL; // our fish feeder emulator class
jmethodID jmethod_command = NULL; // to send commands to the fish feeder emulator
jmethodID jmethod_message = NULL; // to ask the fish feeder emulator for information
jmethodID jmethod_exit = NULL;
jmethodID jmethod_isGUIReady = NULL; // to check if the GUI is ready
jclass jclass_String = NULL; // java string class to pass strings to/from java methods
jclass jclass_Platform = NULL; // java fx Platform class
jmethodID jmethod_platform_exit = NULL; // Platform.exit() method

// thread management we need javaFX and C processing threads
#define MAX_THREADS 2
int threadCount = 0;
pthread_t threads[MAX_THREADS];

#define LINE_SIZE 200
#define NUM_SCROLL_ITEMS_ON_SCREEN 5

#define stringify(x) #x
#define stringify2(x) stringify(x)

#define GUI_OFFSET_DEFAULT_DATE_X(x, i) (16 + (i * x)/4)
#define GUI_OFFSET_DEFAULT_DATE_Y(y, i) ((i * y)/3)
#define GUI_OFFSET_PURPOSE_DATE_Y(y, i) ((i * y)/6)

#define TIME_TILL_SCREEN_TURNS_OFF 10
#define ROTATIONS_LIMIT 6

#define FEED_SCHEDULE_FILE_NAME "FeedScheduler.txt"

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
 * check for java exceptions passed back via the jni
 * quit the program if an exception is found
 * @param env
 * @param msg
 */
void exception_check(JNIEnv *env, const char *msg) {
    char sb[LINE_SIZE]; // string buffer for message

    if ((*env)->ExceptionCheck(env)) {
        snprintf(sb, LINE_SIZE, "Exception occurred in %s", msg);
        logAdd(JNI_MESSAGES, "Exception occurred");

        (*env)->ExceptionDescribe(env); // send to stderr
        exit(1);
    }
}

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
 * add our current C thread 'number' to a string.
 * the pthread_t thread id is an opaque type, so we can't print it directly
 * this qives a consistent number for each thread based on the array position we store the pthread_t items in
 * there must be at least 20 characters of space in the string buffer
 * @param sb a string buffer to add the thread id tor
 * @param length the occupied length of the string (ie position to place result string
 * @return returns the number of characters added to the string
 */
size_t threadId(char *sb, size_t position) {
    pthread_t self = pthread_self();

    for (int i = 0; i <= threadCount; i++) {
        if (pthread_equal(threads[i], self)) {
            position += snprintf(sb+position, 20, "[Thread %2dC] ", i);
        }
    }

    return position;
}
/**
 *
 * @return true if the java GUI is initialised and ready
 */
bool isJavaFXReady(){
    char sb[LINE_SIZE];

    snprintf(sb, LINE_SIZE, "calling %s.%s()", jclass_name_FishFeederEmulator, jmethod_name_isGUIReady);
    logAdd(JNI_MESSAGES,sb);

    // call the java method
    jboolean result = (*env_c)->CallStaticBooleanMethod(env_c, jclass_FishFeederEmulator, jmethod_isGUIReady, NULL);
    exception_check(env_c, jmethod_name_isGUIReady);

    return (bool)result;
}

/**
 * create a new thread (creating a new env_c java thread environment)
 * to run the users C code with an entry point of userProcessing()
 * find the various JVM environment info classes and methods required for this thread
 * @param vargp
 * @return
 */
void* createThread(void * vargp) {
    logAdd(METHOD_ENTRY, "createThread(). C processing thread starting");
    char sb[LINE_SIZE]; // string buffer for messages

    // delay to allow the javaFX thread to initialise (otherwise we likely get an FX uninitialised exception)
    logAdd(JNI_MESSAGES, "delay to allow JavaFX thread to start");
    //sleep(FX_START_DELAY);

    // get the java environment for the C processing thread note env_c is set by this call
    int getEnvStat = (*vm)->GetEnv(vm, (void **) &env_c, JNI_VERSION_9); //TODO update JNI_VERSION_21
    logAdd(JNI_MESSAGES, "got java environment");

    // Attach this thread to the JVM
    // https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html#AttachCurrentThread
    // JNIEnv c. The JNI interface pointer (JNIEnv) is thread specific (only valid in the thread it was obtained in).
    // Should another thread need to access the Java VM, it must first call AttachCurrentThread()
    // to attach itself to the VM and obtain a JNI interface pointer.
    // Once attached to the VM, a native thread works just like an ordinary Java thread running
    // inside a native method. The native thread remains attached to the VM until it calls
    // DetachCurrentThread() to detach itself.
    if (getEnvStat == JNI_EDETACHED) {
        logAdd(JNI_MESSAGES, "getEnv: not attached. Attaching...");
        if ((*vm)->AttachCurrentThread(vm, (void **) &env_c, NULL) != 0) {
            logAdd(JNI_MESSAGES, "processing thread. C: Failed to attach to Java VM");
            exit(1);
        }
    } else if (getEnvStat == JNI_OK) {
        logAdd(JNI_MESSAGES, "JNI already attached to thread");
    } else if (getEnvStat == JNI_EVERSION) {
        logAdd(JNI_MESSAGES, "getEnv: version not supported");
    }

    // hand over to do the users processing.
    // This function should not return until the user code is done with the JFX GUI.
    logAdd(JNI_MESSAGES, "start userProcessing()");

    // wait for JavaFX to be ready.
    // TODO this simple code could hang so might need a timeout
    while (!isJavaFXReady()) {
        logAdd(JFX_MESSAGES, "JavaFX is not ready");
        msleep(50L); // give the GUI thread time to do something!
    }

    // call the application (GUI users code, should not return until the application is finished)
    userProcessing();
    logAdd(JNI_MESSAGES, "returned from userProcessing()... finishing");

    // tell the JFX to exit the process. This will also exit this C processing thread.
    // it is a little ugly but Platform.exit can't detach when the calling thread is the main thread
    // doing it this way also ensures that the C threads are killed when the JFX GUI is closed
    snprintf(sb, LINE_SIZE, "calling java method %s.%s()", jclass_name_FishFeederEmulator, jmethod_name_emulator_exit);
    logAdd(JNI_MESSAGES, sb);
    (*env_c)->CallStaticVoidMethod(env_c, jclass_FishFeederEmulator, jmethod_exit, NULL);

    // clean up - we will never get here using jmethod_name_emulator_exit...
    /*
    (*env_c)->DeleteGlobalRef(env_c, jclass_FishFeederEmulator);
    (*env_c)->DeleteGlobalRef(env_c, class_Platform);
    (*vm)->DetachCurrentThread(vm);

    logAdd(METHOD_ENTRY, "createThread(). Done");
    */
    return NULL;
}

/**
 * get a java method reference using the env_fx thread environment
 * @param class - the java class reference containing the method
 * @param className - the name of the class (used only for error messages)
 * @param methodName - the name of the method we are looking for
 * @param methodSig - the jvm textual method signature
 * @return - the jmethodID reference to the method
 */
jmethodID getJavaMethodReference(jclass class, char const * const className,
                                 char const * const methodName, char const * const methodSig){
    char sb[LINE_SIZE]; // string buffer for messages

    jmethodID method = (*env_fx)->GetStaticMethodID(env_fx, class, methodName, methodSig);

    if (method == NULL) {
        snprintf(sb, LINE_SIZE, "Failed to find java %s.%s() function", className, methodName);
        logAdd(JNI_MESSAGES, sb);
        exit(1);
    }
    return method;
}

/**
 * get a java class reference using the env_fx thread environment
 * @param class_name
 * @return
 */
jclass getJavaClassReference(char const * const class_name) {
    char sb[LINE_SIZE]; // string buffer for messages

    jclass class = (*env_fx)->FindClass(env_fx, class_name);
    if (class == NULL) {
        snprintf(sb, LINE_SIZE, "failed to find java class %str", class_name);
        logAdd(JNI_MESSAGES, sb);
        exit(1);
    }

    // create a java global reference
    // note if we convert a jobject or jclass to a global reference they are valid in any thread (i.e. not only env_fx).
    return (*env_fx)->NewGlobalRef(env_fx, class);
}

/**
 * setup the JNI environment
 * this locates the Java classes and methods required for the C processing thread
 * @return
 */
int jniSetup() {

    // store the main thread id for debugging
    threads[threadCount] = pthread_self(); // store the main thread id

    logAdd(METHOD_ENTRY, "jniSetup(). Start JVM for nns.fishfeedergui");

    // set up the JVM arguments
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_9; // at least java 1.9
    JavaVMOption options[1];
    options[0].optionString = "-Djdk.module.main=nns.fishfeedergui";
    vm_args.options = options;
    vm_args.nOptions = 1;
    vm_args.ignoreUnrecognized = JNI_FALSE;

    // Create (and attach to) the JVM. note env_fx is set by this call
    // https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html#creating_the_vm
    jint jvm_success = JNI_CreateJavaVM(&vm, (void **) &env_fx, &vm_args);

    if (jvm_success != JNI_OK) {
        logAdd(JNI_MESSAGES, "Failed to create Java VM");
        return 1;
    }
    logAdd(JNI_MESSAGES,"jvm successfully started");

    // create a java global reference for the FishFeederEmulator class
    // note if we convert a jobject or jclass to a global reference they are valid in any thread (i.e. not only env_fx).
    jclass_FishFeederEmulator = getJavaClassReference(jclass_name_FishFeederEmulator);

    // create a java global reference for the javafx Platform class
    jclass_Platform = (*env_fx)->FindClass(env_fx, jclass_name_Platform);

    // create a java global reference for the java String class
    jclass_String = (*env_fx)->FindClass(env_fx, jclass_name_String);

    // get the command() method reference
    // note can't create a global reference to jmethodID above (or jfieldID) because they are not jobjects
    // and hence it is safe to store jmethodID in a global variable for use in any thread.
    jmethod_command = getJavaMethodReference(jclass_FishFeederEmulator, jclass_name_FishFeederEmulator,
                                             jmethod_name_command, jmethod_sig_command);

    // get the message method() reference
    jmethod_message = getJavaMethodReference(jclass_FishFeederEmulator, jclass_name_FishFeederEmulator,
                                             jmethod_name_message, jmethod_sig_message);

    // get the Platform.exit() method reference
    jmethod_platform_exit = getJavaMethodReference(jclass_Platform, jclass_name_Platform,
                                                   jmethod_name_platform_exit, jmethod_sig_platform_exit);

    // find the FishFeederEmulator.exit() method name
    jmethod_exit = getJavaMethodReference(jclass_FishFeederEmulator, jclass_name_FishFeederEmulator,
                                          jmethod_name_emulator_exit, jmethod_sig_emulator_exit);

    // find the FishFeederEmulator.isGUIReady() method name
    jmethod_isGUIReady = getJavaMethodReference(jclass_FishFeederEmulator, jclass_name_FishFeederEmulator,
                                                jmethod_name_isGUIReady, jmethod_sig_isGUIReady);

    // Create a new thread to run the C application code
    // This thread will run the users C code with an entry point of userProcessing()
    threadCount++; //next available pthread_t item space
    pthread_create(&threads[threadCount], NULL, createThread, NULL); // thread_id, attr, function, function args

    logAdd(METHOD_ENTRY, "jniSetup(). Done");
    return 0;
}

/**
 * initialise and start the JavaFX application
 * must only be called in the javafx application thread (env_fx) environment
 * this thread will be handed over to the JavaFX application and will not return until
 * the JavaFX application is closed
 * @return 0 if successful 1 if unsuccessful
 */
int javaFx() {
    char sb[LINE_SIZE]; // string buffer for messages
    logAdd(METHOD_ENTRY, "javaFx()");

    // get the main method reference.
    jmethodID method_main = getJavaMethodReference(jclass_FishFeederEmulator,
                                                   jclass_name_FishFeederEmulator,
                                                   jmethod_name_main, jmethod_sig_main);

    // Call the main java method with the log level as the parameter
    snprintf(sb, LINE_SIZE, "calling %s.%s()", jclass_name_FishFeederEmulator, jmethod_name_main);
    logAdd(JNI_MESSAGES,sb);

    char *str = malloc(sizeof(char) * LINE_SIZE);
    sprintf(str, "%d", log_level);
    jstring jstr = (*env_fx)->NewStringUTF(env_fx, str);
    jobjectArray args = (*env_fx)->NewObjectArray(env_fx, 1, jclass_String, jstr); // 1 item

    (*env_fx)->CallStaticVoidMethod(env_fx, jclass_FishFeederEmulator, method_main, args);
    exception_check(env_fx, jmethod_name_main);

    // free java object memory
    //(*env_c)->DeleteLocalRef(env_fx, jstr); //BUG!
    //(*env_c)->DeleteLocalRef(env_fx, args);
    (*env_fx)->DeleteLocalRef(env_fx, jstr);
    (*env_fx)->DeleteLocalRef(env_fx, args);

    // without the while will not normally get here (in *nix platforms) until the JavaFX windows close
    // for windows we need to keep the thread alive...
    // TODO hence we rely on process exit() to terminate the application (all threads)
    // this has the advantage that the entire process will exit regardless of
    // whether the Java GUI or C thread dies/is killed and will save students from
    // having lots of half dead processes hanging around when code fails.
    while(1){}

    //(*env_fx)->CallStaticVoidMethod(env_fx, jclass_Platform, jmethod_platform_exit, args);
    //(*vm)->DetachCurrentThread(vm); // causes Toolkit not initialized exception
    //(*vm)->DestroyJavaVM(vm);
    //logAdd(METHOD_ENTRY, "javaFx() Done");
    //return 0;
}

/**
 * call the java command method with an array of strings argument provided.
 * @param jargs
 */
void call_j_command (jobjectArray jargs) {
    logAdd(JNI_MESSAGES, "calling java command function");
    (*env_c)->CallStaticObjectMethod(env_c, jclass_FishFeederEmulator, jmethod_command, jargs);
    exception_check(env_c, jmethod_name_command);
    logAdd(JNI_MESSAGES, "returned from java command function");

    // release the local array reference
    (*env_c)->DeleteLocalRef(env_c, jargs);
}

/**
 * send a message to the JavaFX application and get a response
 * @param message
 * @return the response message - memory needs freeing by caller
 */
char *call_j_message(jobjectArray jargs) {
    //logAdd(METHOD_ENTRY, "call_j_message()")

    char sb[LINE_SIZE];
    logAdd(JNI_MESSAGES, "calling java message function");
    jstring jstr_result = (*env_c)->CallStaticObjectMethod(env_c, jclass_FishFeederEmulator, jmethod_message, jargs);
    exception_check(env_c, jmethod_name_message);
    logAdd(JNI_MESSAGES, "returned from java message function");

    (*env_c)->DeleteLocalRef(env_c, jargs);

    // get the result string as a c string pointer from the java object
    const char *cstr_result = (*env_c)->GetStringUTFChars(env_c, jstr_result, NULL);

    char *result = malloc(LINE_SIZE);
    strcpy(result, cstr_result);
    // release the java result string memory (must copy the string first if we want to keep it)
    (*env_c)->ReleaseStringUTFChars(env_c, jstr_result, cstr_result);

    snprintf(sb, LINE_SIZE, "result '%s'", result);
    logAdd(JNI_MESSAGES, sb);

    //logAdd(METHOD_ENTRY, "call_j_message() Done");
    return result;
}

/**
 * create a list of jni arguments from a set of parameters
 * a format specifier must be used as the first parameter
 * s character signifies a string argument in the respective position
 * d character signifies a integer argument in the respective position
 * f character signifies a floating point type argument in the respective position
 * @param format
 * @param ...
 * @return a jni jobjectArray
 */
jobjectArray build_args(char *format, ...) {
    va_list args;
    va_start(args, format);

    // convert the parameters into strings that will be passed to the java method as an array of strings
    jstring jstrs;

    // init jni parameter list
    jstring init = (*env_c)->NewStringUTF(env_c, "");
    jobjectArray jargs = (*env_c)->NewObjectArray(env_c, (jsize)strlen(format), jclass_String, init); // 3 items

    // process each parameter
    char *str = malloc(sizeof(char) * LINE_SIZE);
    int count = 0;
    while (*format != '\0') {
        switch (*format++) {
            case 's':
                sprintf(str, "%s", va_arg(args, const char *));
                //fprintf(stdout, "string arg[%d]: %s\n", count, str);
                jstrs = (*env_c)->NewStringUTF(env_c, str);
                break;
            case 'd':
                sprintf(str, "%d", va_arg(args, int));
                //fprintf(stdout, "string arg[%d]: %s\n", count, str);
                jstrs = (*env_c)->NewStringUTF(env_c, str);
                break;
            case 'l':
                sprintf(str, "%ld", va_arg(args, long));
                //fprintf(stdout, "string arg[%d]: %s\n", count, str);
                jstrs = (*env_c)->NewStringUTF(env_c, str);
                break;
            case 'f':
                sprintf(str, "%f", va_arg(args, double));
                //fprintf(stdout, "string arg[%d]: %s\n", count, str);
                jstrs = (*env_c)->NewStringUTF(env_c, str);
                break;
        }
        // add the next jni parameter
        (*env_c)->SetObjectArrayElement(env_c, jargs, count, jstrs);
        (*env_c)->DeleteLocalRef(env_c, jstrs);
        count += 1;
    }

    free(str);
    va_end(args);

    return jargs;
}

/**
 * send message to the JavaFX application
 * to clear the display
 */
void displayClear() {
    call_j_command(build_args("s", "CLEAR_DISPLAY"));
}

/**
 * send message to the JavaFX application
 * to step the motor
 */
void motorStep() {
    call_j_command(build_args("s", "MOTOR_STEP")); // 1st argument is format specifier
}

/**
 * send message to the JavaFX application
 * to clear a specified region of the display
 * @param x
 * @param y
 * @param w
 * @param h
 */
void displayClearArea(int x, int y, int w, int h) {
    call_j_command(build_args("sdddd", "CLEAR_AREA", x, y, w, h)); // 1st argument is format specifier
}

/**
 * send message to the JavaFX application
 * to display a line on the JavaFX application display
 * @param x
 * @param y
 * @param w
 * @param h
 */
void displayLine(int x, int y, int w, int h) {
    call_j_command(build_args("sdddd", "LINE", x, y, w, h)); // 1st argument is format specifier
}

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
    call_j_command(build_args("sdddddd", "SET_RTC", sec, min, hour, day, month, year)); // 1st argument is format specifier
}

/**
 * send message to the JavaFX application
 * to display a pixel on the JavaFX application display
 * @param x
 * @param y
 */
void displayPixel(int x, int y) {
    call_j_command(build_args("sdd", "PIXEL", x, y)); // 1st argument is format specifier
}

/**
 * fill the food container to a percentage and
 * send message to the JavaFX application
 * @param foodLevel
 */
void foodFill(int foodLevel) {
    call_j_command(build_args("sd", "FOOD", foodLevel)); // 1st argument is format specifier
}

/**
 * display text string on the JavaFX application display
 * @param x - x pixel position
 * @param y - x pixel position
 * @param text - the text to display
 * @param size - 1 or 2 are the only two sizes currently supported on the real display
 */
void displayText(int x, int y, char *text, int size) {
    call_j_command(build_args("sddsd", "TEXTXY", x, y, text, size)); // 1st argument is format specifier
}

/**
 * send an information message to the JavaFX application info area
 * @param text
 */
void infoMessage(char *text) {
    call_j_command(build_args("ss", "MESSAGE", text)); // 1st argument is format specifier
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
    call_j_command(build_args("sss", "COLOUR", fg, bg)); // 1st argument is format specifier
}

/**
 * send a message to the JavaFX application and get a response
 * @return
 */
char *buttonState() {
    //Called to reset the last time the button was pressed so we can turn the screen off after a certain amount of time
    //*timeToTurnOfPtr = clockSecond();

    return call_j_message(build_args("s", "BUTTON")); // 1st argument is format specifier();
}

/**
 * convert string to long, checking for invalid numbers
 * @return
 */
long convertStringToLong(char *str) {
    char *end;
    long result = strtol(str, &end, 10);

    if (*end != '\0') {
        logAdd(JNI_MESSAGES, "convertStringToLong() invalid number");
        result = -1;
    }

    return result;
}

char *convertLongToString(long value) {
    //char *convertedStr = ultoa();
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
long long int clockWarmStart(long long int offset) {
    char *resultstr =  call_j_message(build_args("sl", "RTC_WARM_START", offset)); // 1st argument is format specifier();
    long result = convertStringToLong(resultstr);
    free(resultstr);
    return result;
}

/**
 * send a time item message to the JavaFX application and get a response
 * @param item
 * @return
 */
int clockitem(char *item) {
    char *resultstr =  call_j_message(build_args("s", item)); // 1st argument is format specifier();
    int result = (int)convertStringToLong(resultstr);
    free(resultstr);
    return result;
}

int clockSecond() {
    return clockitem("RTC_SECOND");
}

int clockMinute() {
    return clockitem("RTC_MINUTE");
}

int clockHour() {
    return clockitem("RTC_HOUR");
}

int clockDay() {
    return clockitem("RTC_DAY");
}

int clockMonth() {
    return clockitem("RTC_MONTH");
}

int clockYear() {
    return clockitem("RTC_YEAR");
}

int clockDayOfWeek() {
    return clockitem("RTC_DAY_OF_WEEK");
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

        // add thread info if needed
        if ((log_level & THREAD_ID ) > 0) {
            position = threadId(sb, position);
        }

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

 void getAllDatesFromFileAsString(char **timesListPtr, char *filename) {
    //Make a file pointer that points to the file location of the file we want to read from
    FILE *file = fopen(filename, "r");
    //Error check for if we cant find the file
    if (file == NULL) {
        printf("Error opening the feed times file");
    }else {
        int i = 0;
        while (!feof(file)) {
            char *temp1[20];
            char *temp2[20];
            char *temp3[20];

            fscanf(file, "%s %s:%s", temp1, temp2, temp3);
            sprintf(timesListPtr[i], "%s", temp2);
            i++;
        }
    }
}

/**
 * This function outputs the feed schedule into the feedTimes ptr array which has already been allocated memory from the heap
 * @param feedTimes FeedTimes list where the schedule will be stored
 * @param filename Name of the file as a string
 */
void getClosestDateFromFile(FeedTime *feedTime, char *filename) {
    //Make a file pointer that points to the file location of the file we want to read from
    FILE *file = fopen(filename, "r");
    //Error check for if we cant find the file
    if (file == NULL) {
        printf("Error opening the feed times file");
    }else {
        int isFirstRun = 1;
        while (!feof(file)) {
            FeedTime tempTime;

            fscanf(file, "%d %d:%d", &tempTime.numRots, &tempTime.hour, &tempTime.minute);

            //We set the intial value of feedTime to the first read value from the file as if there is only one value in the file it the program doesn't need to compare ant times
            if (isFirstRun) {
                *feedTime = tempTime;
                isFirstRun = 0;
            }else {
                //Making feedTime the closest time in the feed schedule to the current time making it the next time
                if (tempTime.hour >= clockHour() && tempTime.hour <= feedTime->hour) {
                    if (tempTime.minute > clockMinute() && tempTime.minute < feedTime->minute) {
                        feedTime->hour = tempTime.hour;
                        feedTime->minute = tempTime.minute;
                        feedTime->numRots = tempTime.numRots;
                    }
                }
            }
        }
    }
}

//The theory for getting the following feed time is to effectively run the method twice, the first time we retreive the next closest feed time, the second time we know the
// next closest feed time so we look for the closest feed time again but this time don't check if the feed time is the one we've already found
void getNextClosestDateFromFile(FeedTime *feedTime, char *filename) {
    //Make a file pointer that points to the file location of the file we want to read from
    FILE *file = fopen(filename, "r");

    //Getting the closest feed time
    FeedTime *closestTime = malloc(sizeof(FeedTime));
    getClosestDateFromFile(closestTime, filename);

    //Error check for if we cant find the file
    if (file == NULL) {
        printf("Error opening the feed times file");
        //Add screen error here
    }else {
        //Using the closest feed time to find the second-closest feed time
        int isFirstRun = 1;
        while (!feof(file)) {
            FeedTime tempTime;

            fscanf(file, "%d %d:%d", &tempTime.numRots, &tempTime.hour, &tempTime.minute);

            //We set the intial value of feedTime to the first read value from the file as if there is only one value in the file it the program doesn't need to compare ant times
            if (isFirstRun) {
                *feedTime = tempTime;
                isFirstRun = 0;
            }else {
                //Making feedTime the closest time in the feed schedule to the current time making it the next time
                if (tempTime.hour >= closestTime->hour && tempTime.hour <= feedTime->hour) {
                    if (tempTime.minute > closestTime->minute && tempTime.minute < feedTime->minute) {
                        feedTime->hour = tempTime.hour;
                        feedTime->minute = tempTime.minute;
                        feedTime->numRots = tempTime.numRots;
                    }
                }
            }
        }
    }

    free(closestTime);
}

int howManyLinesInFile(char* filename) {
    //Make a file pointer that points to the file location of the file we want to read from
    FILE *file = fopen(filename, "r");
    int counter = 0;
    //Error check for if we cant find the file
    if (file == NULL) {
        printf("Error opening the feed times file");
        //Add screen error here
    }else {
        while (!feof(file)) {
            if(fgetc(file) == '\n') {
                counter++;
            }
        }
    }

    return counter;
}

void displayNumberOfFeeds(int x, int y, int numFeeds, int numFeedsSize) {
    char numFeedsString[100];
    sprintf(numFeedsString, "No. feeds:%d", numFeeds);

    //Display no. feeds since booted up
    displayText(x, y, numFeedsString, numFeedsSize);
}

//Displays a scrollbar using the parameters provided where each item in the items array is displayed as a value that can be scroller past in
// the scrolling area
void displayScroller(char** items, int *rangeIndex, int x, int y, int size, int *scrollOffset) {
    //Remove the previously displayed values as they are a scroll index behind now
    if (*rangeIndex >= size) {
        *rangeIndex = 0;
        *scrollOffset = 0;
    }

    //Display the lines around the scroller
    displayLine(x/32, y/6, x/32, (15*y)/16);
    displayLine((31*x)/32, y/6, (31*x)/32, (15*y)/16);

    //5 is the most elements that can be displayed on screen
    if (size > 5) {
        size = 5;
    }

    //Displays all the items on screen inside the scroller
    displayClearArea(x/32, y/6, (31*x)/32, (15*y)/16);
    for (int i = 0; i < size; i++) {
        //If there are more items to look at but not enough space on screen we just apply the scrollOffset
        if (items[i + *scrollOffset] != NULL) {
            //Shows which item is currently selected
            if ((*rangeIndex) == (i + *scrollOffset)) {
               displayTextHighlighted(x/16, (y*(i + 1))/6, items[i + *scrollOffset], 1);
            }else {
                displayText(x/16, (y*(i + 1))/6, items[i + *scrollOffset], 1);
            }
        }
    }
}

void boundTimeVals(int *currentTSI, int *currentTSV) {
    //Check that the hours make sense
    if (*currentTSI == 0 && *currentTSV > 24) {
        *currentTSV = 0;
    }

    //Check that the minutes make sense
    if (*currentTSI == 1 && *currentTSV > 60) {
        *currentTSV = 0;
    }
}

void displayDefaultDateValues(int x, int y, int numVals, FeedTime *valsToWrite) {
    char* valuePurposes[3] = {"hours", "mins", "rotations"};

    //print the values to the GUI
    for (int i = 0; i < numVals; i++) {
        int writingVal = 0;
        if (i == 0) {
            writingVal = valsToWrite->hour;
        }else if (i == 1) {
            writingVal = valsToWrite->minute;
        }else if (i == 2) {
            writingVal = valsToWrite->numRots;
        }

        displayText(GUI_OFFSET_DEFAULT_DATE_X(x, i), GUI_OFFSET_PURPOSE_DATE_Y(y, 1), valuePurposes[i], 1);
        char *toString[20];
        if (writingVal > 10) {
            sprintf(toString, "%d", writingVal);
        }else if (writingVal != 0 && i != 2){
            sprintf(toString, "0%d", writingVal);
        }else {
            sprintf(toString, "%d", writingVal);
        }
        displayText(GUI_OFFSET_DEFAULT_DATE_X(x, i), GUI_OFFSET_DEFAULT_DATE_Y(y, 1), toString, 1);
    }
}

void displayDefaultClockValues(int x, int y, int numVals, ClockTime *valsToWrite) {
    char* valuePurposes[6] = {"sec", "min", "hour", "day", "month", "year"};

    //print the values to the GUI
    for (int i = 0; i < numVals; i++) {
        int writingVal = 0;
        if (i == 0) {
            writingVal = valsToWrite->second;
        }else if (i == 1) {
            writingVal = valsToWrite->minute;
        }else if (i == 2) {
            writingVal = valsToWrite->hour;
        }else if (i == 3) {
            writingVal = valsToWrite->day;
        }else if (i == 4) {
            writingVal = valsToWrite->month;
        }else if (i == 5) {
            writingVal = valsToWrite->year;
        }

        //levelChecker is responsible for making sure all the information for the number picker can be displayed within the screen
        //We do this by making the GUI print the remaining values a level below the first ones
        int levelChecker = 1;

        if (i > 2) {
            levelChecker = 2;
        }

        if (levelChecker == 1) {
            displayText(GUI_OFFSET_DEFAULT_DATE_X(x, (i - ((levelChecker - 1) * 3))), GUI_OFFSET_PURPOSE_DATE_Y(y, levelChecker), valuePurposes[i], 1);
        }else {
            displayText(GUI_OFFSET_DEFAULT_DATE_X(x, (i - ((levelChecker - 1) * 3))), GUI_OFFSET_PURPOSE_DATE_Y(y, 1.5 * levelChecker), valuePurposes[i], 1);
        }

        char *toString[20];
        if (writingVal > 10) {
            sprintf(toString, "%d", writingVal);
        }else if (writingVal != 0){
            sprintf(toString, "0%d", writingVal);
        }else {
            sprintf(toString, "%d", writingVal);
        }
        displayText(GUI_OFFSET_DEFAULT_DATE_X(x, (i - ((levelChecker - 1) * 3))), GUI_OFFSET_DEFAULT_DATE_Y(y, levelChecker), toString, 1);
    }
}

/**
 *
 * @param x
 * @param y
 * @param currentTSI
 * @param currentTSV
 * @param feedTimeValsToSave
 * @return If true is returned then we are finished using the number pickers, if false is returned then the number pickers are still in use
 */
void displayNumberPicker(int x, int y, int *currentTSI, int *currentTSV) {
    //Highlight the current value the user is selecting
    char* valuePickedAsString[20];

    //levelChecker is responsible for making sure all the information for the number picker can be displayed within the screen
    //We do this by making the GUI print the remaining values a level below the first ones
    int levelChecker = 1;

    if (*currentTSI > 2) {
        levelChecker = 2;
    }

    sprintf(valuePickedAsString, "%d", *currentTSV);
    displayTextHighlighted(GUI_OFFSET_DEFAULT_DATE_X(x, (*currentTSI - ((levelChecker - 1) * 3))), GUI_OFFSET_DEFAULT_DATE_Y(y, levelChecker), valuePickedAsString, 1);
}


char *intToModeOptionConverter(int x) {
    if (x == 0) {
        return "Paused";
    }
    if (x == 1) {
        return "Auto";
    }
    if (x == 2) {
        return "Feed now";
    }
    if (x == 3) {
        return "Skip next";
    }
    if (x == 4) {
        return "Auto";
    }

    return NULL;
}

void displayTitle(char* title) {
    displayColour("white", "black"); // white text on black background
    displayText(0,0, title, 2);
}

void displayTextHighlighted(int x, int y, char* text, int textSize) {
    displayColour("white", "blue");
    displayText(x, y, text, textSize);
    displayColour("white", "black");
}

void parseTimeToFile(FeedTime *timesToSave, char * filename) {
    FILE *fileptr;

    fileptr = fopen(filename, "a");

    if (timesToSave->hour < 10 && timesToSave->minute < 10) {
        fprintf(fileptr, "\n%d 0%d:0%d", timesToSave->hour, timesToSave->minute, timesToSave->numRots);
    }else if (timesToSave->hour < 10) {
        fprintf(fileptr, "\n%d 0%d:%d", timesToSave->hour, timesToSave->minute, timesToSave->numRots);
    }else if (timesToSave->minute < 10) {
        fprintf(fileptr, "\n%d %d:0%d", timesToSave->hour, timesToSave->minute, timesToSave->numRots);
    }else {
        fprintf(fileptr, "\n%d %d:%d", timesToSave->hour, timesToSave->minute, timesToSave->numRots);
    }


    fclose(fileptr);
}

void removeTimeFromFile(char *timesToRemove, char * filename) {
    //Make a file pointer that points to the file location of the file we want to read from
    FILE *file = fopen(filename, "r");

    char *fileAsString = malloc(sizeof(char*));

    //Error check for if we cant find the file
    if (file == NULL) {
        printf("Error opening the feed times file");
        //Add screen error here
    }else {
        //Using the closest feed time to find the second-closest feed time
        //The alternator makes sure the computer doesn't read the rotations as a separate piece of data
        int i = 0;
        int alternator = 1;
        char *rotationsToWrite[20];
        while (!feof(file)) {
            char tempTime[20];

            char *lineOfFile[20];
            fscanf(file, "%s", lineOfFile);
            if (alternator % 2 == 0) {
                sscanf(lineOfFile, "%s", &tempTime);

                //Add all the scheduled times except the one we want to remove. We don't worry about checking rotations as the time given doesn't include them
                if (strcmp(tempTime, timesToRemove) == 0) {
                    //This is the file we want to remove so we don't do anything
                }else {

                    //Write to fileAsString to get ready to be written onto the file
                    strcat(rotationsToWrite, lineOfFile);
                    strcpy(lineOfFile, rotationsToWrite);
                    strcat(lineOfFile, "\n");
                    strcat(fileAsString, lineOfFile);

                    printf("\n");
                    printf("\n");
                    printf("%s\n", lineOfFile);
                    printf("\n");
                    printf("\n");
                }
            }else { //This is the number of rotations
                sscanf(lineOfFile, "%s", &rotationsToWrite);
                strcat(rotationsToWrite, " ");
            }

            i++;
            alternator++;
        }
    }

    //I know the memory here should be freed but for some very strange if I uncomment it the entire program crashes so uncomment at your own risk
    //fclose(file);

    //Write to the file with all the scheduled times with the one we don't want removed
    FILE *fileptr;

    fileptr = fopen(filename, "w");
    fprintf(fileptr, "%s", fileAsString);

    fclose(fileptr);

    //Same here as with the fclose(file) problem. Doesn't make sense but I spent a while finding it, did this, then it worked
    //free(fileAsString);
}

void motorDisplay(int x, int y, int motorTurn) {
    displayClear();
    displayColour("white", "black");

    if (motorTurn % 30 < 10) {
        displayText(x,y, "Feeding...", 2);
    }else if (motorTurn % 20 < 10) {
        displayText(x,y, "Feeding..", 2);
    }else if (motorTurn % 10 < 10){
        displayText(x,y, "Feeding.", 2);
    }else {
        displayText(x,y, "Feeding", 2);
    }
}

void nextFeedTimeToString(char *timeString, FeedTime *timeptr) {
    if (timeptr->minute < 10) {
        sprintf(timeString, "%d:0%d", timeptr->hour, timeptr->minute);
    }else {
        sprintf(timeString, "%d:%d", timeptr->hour, timeptr->minute);
    }
}

/**
 * this was the original version before the build_args function with variable length
 * parameter lists was created. All of the messaging functions had a similar code fragment
 * adapted for the number and type of parameters. They could become one line following the
 * including of the build_args function.
 * send message to the JavaFX application
 * to display a pixel on the JavaFX application display
 * @param x
 * @param y

void displayPixel_ORIGINAL(int x, int y) {
    char *str0 = malloc(sizeof(char) * 80);
    sprintf(str0, "PIXEL");
    jstring jstr0 = (*env_c)->NewStringUTF(env_c, str0);

    char *str1 = malloc(sizeof(char) * 80);
    sprintf(str1, "%d", x);
    jstring jstr1 = (*env_c)->NewStringUTF(env_c, str1);

    char *str2 = malloc(sizeof(char) * 80);
    sprintf(str2, "%d", y);
    jstring jstr2 = (*env_c)->NewStringUTF(env_c, str2);

    jobjectArray args = (*env_c)->NewObjectArray(env_c, 3, jclass_String, jstr0); // 3 items
    (*env_c)->SetObjectArrayElement(env_c, args, 1, jstr1);
    (*env_c)->SetObjectArrayElement(env_c, args, 2, jstr2);

    logAdd(JNI_MESSAGES, "calling java message function with 'PIXEL'");
    (*env_c)->CallStaticObjectMethod(env_c, jclass_FishFeederEmulator, jmethod_command, args);
    exception_check(env_c, jmethod_name_command);
    logAdd(JNI_MESSAGES, "returned from java message function");

    (*env_c)->DeleteLocalRef(env_c, args);
    (*env_c)->DeleteLocalRef(env_c, jstr0);
    (*env_c)->DeleteLocalRef(env_c, jstr1);
    (*env_c)->DeleteLocalRef(env_c, jstr2);

    //release the c string
    free(str0);
    free(str1);
    free(str2);
}*/
