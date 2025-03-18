cmake_minimum_required(VERSION 3.25)
project(2024_2025_fish_C C)

set(CMAKE_C_STANDARD 11)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories (
		# jni.h header file location
		# jni_md.h header file location
		
        # macos
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include/darwin

        # linux
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include/linux

        # windows
        ${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include
        ${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/include/win32
)

add_executable(2024_2025_fish_C main.c fish.c fish.h)

target_link_libraries(2024_2025_fish_C)

target_link_libraries (
        ${PROJECT_NAME} PUBLIC

        # macos
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/lib/libjli.dylib
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/lib/server/libjvm.dylib

        # linux
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/lib/libjli.so
        #${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/lib/server/libjvm.so

        # windows
        ${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/lib/jvm.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/FishFeederGUI/customjre/bin/server/jvm.dll
        # IMPORTANT in addition to the lines above in CLion edit run configuration and
        # set an Environment Variable for Path to allow the jvm.dll to be found in the custom jre as follows:
        # Path=$PROJECT_DIR$\FishFeederGUI\customjre\bin\server\

        # if running on the command line in windows set paths in terminal to find cygwin1.dll and the jvm.dll
        # set PATH=C:\path_to_project\2024_2025_fish_C\FishFeederGUI\customjre\bin\server
        # set PATH=%PATH%;C:\cygwin64\bin
)
