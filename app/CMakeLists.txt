# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(pathToProject /home/ptramontin/workspace/DetectCardNumber)

set(CMAKE_VERBOSE_MAKEFILE on)

# Specifies a path to native header files.
include_directories(src/main/cpp/card.io-dmz/)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp )

add_library ( opencv_core SHARED IMPORTED )

add_library ( opencv_imgproc SHARED IMPORTED )

set_target_properties(opencv_core PROPERTIES IMPORTED_LOCATION ${pathToProject}/app/src/main/cpp/lib/${ANDROID_ABI}/libopencv_core.so)
set_target_properties(opencv_imgproc PROPERTIES IMPORTED_LOCATION ${pathToProject}/app/src/main/cpp/lib/${ANDROID_ABI}/libopencv_imgproc.so)


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       opencv_core
                       opencv_imgproc

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
