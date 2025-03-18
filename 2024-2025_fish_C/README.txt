This is the C source folder for an example test of the fish feeder GUI.

The subfolder FishFeederGUI must contain the customjre subfolder for the platform 
you are using. There are versions for the main platforms and processor architectures:

Linuxx64
Linuxaarch64
MacOSaarch64
MacOSx64
Windowsx64

These are downloadable as zip files
customjre_*
where * will be of the above architectures.
I have not yet built the customjre for Linux on an Arm architecture if anyone needs this please ask and
we will build it.

The CMakeLists.tst file has two elements that must be set dependent on the platform you are using:

include_directories
target_link_libraries

You need to uncomment the entries for the platform and comment out or delete the other entries.

Windows also requires a path to these libraries to be set either in the run configuration in CLion
(or as a path entry if the program is to be executed from the terminal) to prevent it finding
incompatible versions of the jvm components if java is installed elsewhere in the system.
*nix based platforms naturally look first in the provided custom runtime folder so don't need
any additional paths to be set.

The relevant files to view and run:

. fish.c : Contains useful functions that are used by main.c through the fish.h header file.
. fish.h : Contains a reference to all the relevant functions in fish.c
. main.c : The main file to run. Contains the menu methods and references to the fish.h file and the libraries:
  <stdio.h> / <stdlib.h> / <string.h> / <stdbool.h> / <_cygwin.h>

 The fish.c and main.c files contain various pre-processor variables and statements to aid the program in various ways

 In order to build the code you need to have compatible software. You have to have a linux/unix compiler on your device,
 for apple or linux users you shouldn't need to outsource software but for windows users you should install cygwin with a
 recent version for maximum compatibility. Once this is achieved you should be able to build and run the project without
 problems and start by running the main() file in main.c


