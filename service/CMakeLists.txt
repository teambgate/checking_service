#
# Copyright (C) 2017 Manh Tran
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
cmake_minimum_required(VERSION 2.8.9)
project (service C)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(MACOSX TRUE)
elseif (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
        add_definitions(-Dlinux)
        set(LINUX TRUE)
endif()

if(${APP_DEBUG} MATCHES "true")
        add_definitions(-DAPP_DEBUG=1)
else()
        add_definitions(-DAPP_DEBUG=0)
endif()

if(${APP_LOG} MATCHES "true")
        add_definitions(-DAPP_LOG=1)
else()
        add_definitions(-DAPP_LOG=0)
endif()

add_definitions(-Wno-incompatible-pointer-types)

add_definitions(-D__int64=long\ long)

add_subdirectory(../../cherry/kernel ../${OUT}/kernel)
add_subdirectory(../../cherry/xml ../${OUT}/xml)
add_subdirectory(../../cherry/server ../${OUT}/server)
add_subdirectory(../../cherry/smartfox ../${OUT}/smartfox)

include_directories(../../cherry/include)
include_directories(../../cherry/smartfox/include)
include_directories(../common/include)
include_directories(../s2/include)
include_directories(include)


# LINKS JAVA LIBRARIES
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        include_directories(/usr/lib/jvm/java-8-oracle/include/)
        include_directories(/usr/lib/jvm/java-8-oracle/include/linux)
        link_directories(/usr/lib/jvm/java-8-oracle/jre/lib/amd64/server)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        include_directories(/System/Library/Frameworks/JavaVM.framework/Headers)
        link_directories(/Library/Java/JavaVirtualMachines/jdk1.8.0_131.jdk/Contents/Home/jre/lib/server)
elseif (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
		include_directories(/usr/include/curl)
        include_directories(/cygdrive/c/Program\ Files/Java/jdk1.8.0_77/include)
        include_directories(/cygdrive/c/Program\ Files/Java/jdk1.8.0_77/include/win32)
        link_directories(/cygdrive/c/Program\ Files/Java/jdk1.8.0_77/jre/bin/server)
endif()

# ADD SOURCES RECURSIVELY
if(MACOSX)
        file(GLOB_RECURSE SOURCES "src/*.c" "src/*.m" "../common/src/*.c" "../s2/src/*.c")
        set(PROJECT_LINK_LIBS kernel xml server smartfox jvm curl)
else()
        file(GLOB_RECURSE SOURCES "src/*.c" "../common/src/*.c" "../s2/src/*.c")
        set(PROJECT_LINK_LIBS kernel xml server smartfox jvm curl)
endif()

add_executable(service ${SOURCES})
add_dependencies(service ${PROJECT_LINK_LIBS})
target_link_libraries(service ${PROJECT_LINK_LIBS})
