# LibreSprite Desktop Integration Module
# Copyright (C) 2016  Gabriel Rauter
#
# Licensed under the the MIT License (https://opensource.org/licenses/MIT).

cmake_minimum_required(VERSION 2.8.12)
set(QT_MIN_VERSION "5.2.0")
project(librespritethumbnail)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
set(KDE_INSTALL_DIRS_NO_DEPRECATED, TRUE)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
find_package(KF5 REQUIRED COMPONENTS KIO)
add_definitions(${QT_DEFINITIONS} -DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
set(KDE_INSTALL_USE_QT_SYS_PATHS)
add_library(librespritethumbnail MODULE aseprite_thumb_creator.cpp)
target_link_libraries(librespritethumbnail KF5::KIOWidgets)
install(TARGETS librespritethumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES librespritethumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
