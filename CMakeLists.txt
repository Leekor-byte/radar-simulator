cmake_minimum_required(VERSION 3.16)

project(radar-simulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    ui/mainwindow.ui

    core/RadarCalculator.cpp
    core/RadarCalculator.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(radar-simulator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(radar-simulator SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(radar-simulator
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(radar-simulator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(radar-simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/core
)

set_target_properties(radar-simulator PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(radar-simulator)
endif()