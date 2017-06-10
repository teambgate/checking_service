# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_definitions(-DAPP_DEBUG=1)
add_definitions(-DGFX=OGL -DIMAGE_TYPE=IMAGE_DROID)
add_definitions(-DUSER_BUILD_ANDROID=true)

include_directories(../../../cherry/include)
include_directories(../../../cherry/smartfox/include)
include_directories(../../../checking_service/common/include)
include_directories(../../../native_ui/include)
include_directories(../../../cherry/android_lib/libzip)

file(GLOB_RECURSE SOURCES
    "../../../cherry/android_lib/libzip/*.c"
    "../../../cherry/kernel/*.c"
    "../../../cherry/xml/*.c"
    "../../../cherry/smartfox/*.c"
    "../../../checking_service/common/*.c"
    "../../../native_ui/src/*.c"
    )

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp
             ${SOURCES}
             )

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

find_library( # Sets the name of the path variable.
              android-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              android)

find_library( # Sets the name of the path variable.
              z-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              z)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}

                       ${android-lib}

                       ${z-lib})