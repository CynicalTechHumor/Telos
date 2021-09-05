#    This file is part of Telos
#    Copyright (c) 2021, Cynical Tech Humor LLC

#    Telos is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

#    Telos is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with Telos.  If not, see <https://www.gnu.org/licenses/>.

#    Source code is available at:
#    <https://github.com/CynicalTechHumor/Telos>

cmake_minimum_required(VERSION 3.5)

project(Telos VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
        task.cpp
        task.h
        dialogtaskselect.cpp
        dialogtaskselect.h
        dialogtaskselect.ui
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        main.cpp
        resource.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Telos WIN32
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(Telos SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(Telos
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Telos PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(Telos PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Telos)
endif()
