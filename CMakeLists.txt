# SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16)
project(kwin-arhud VERSION 0.1 LANGUAGES C CXX)

set(QT_MIN_VERSION "6.5.2")
set(KF_MIN_VERSION "6.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

include(FeatureSummary)
include(GenerateExportHeader)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

find_package(QtWaylandScanner REQUIRED)
set_package_properties(QtWaylandScanner PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required for building this effect"
)

find_package(Wayland 1.20 REQUIRED)
set_package_properties(Wayland PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required for building this effect"
)

add_definitions(-DQT_NO_KEYWORDS)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    WindowSystem
)
find_package(epoxy 1.3 REQUIRED)
find_package(KWin REQUIRED)
find_package(MBitionWaylandProtocols REQUIRED)


set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

# Use Mbient common modules
find_package(MbientCommon REQUIRED >= 0.0.17)
# Get compiler flags from MBient common library
include(CompileConfig)

# Removing flags with violate AUTOSAR C++ guidelines
list(REMOVE_ITEM AVAILABLE_CXX_COMPILATION_FLAGS -Wsuggest-attribute=pure
                 -Wsuggest-attribute=const -Wsuggest-attribute=noreturn
)

set(ENABLE_SECURE_COMPILATION ON)

# cc1plus: warning: '-Werror=' argument '-Werror=bad-function-cast' is not valid for C++
list(REMOVE_ITEM SECURE_CXX_COMPILATION_FLAGS -Werror=bad-function-cast)

# Additional flags
list(APPEND SECURE_CXX_COMPILATION_FLAGS
  -O # Needed by -D_FORTIFY_SOURCE=2
  -Werror=old-style-cast # Remove C-style cast according to AUTOSAR A5-2-2
  -Werror=switch-enum # error if switch case paths are missing for enum values
  -fPIC
)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
