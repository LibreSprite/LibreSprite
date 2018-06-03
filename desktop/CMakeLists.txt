# LibreSprite Desktop Integration Module
# Copyright (C) 2016  Gabriel Rauter
# Copyright (C) 2016  David Capello
# Copyright (C) 2016  Matthias Mailänder
#
# Licensed under the the MIT License (https://opensource.org/licenses/MIT).

if(UNIX AND NOT APPLE)
  # Desktop shortcut
  install(FILES libresprite.desktop
          DESTINATION share/applications)

  # Package gallery integration
  install(FILES libresprite.appdata.xml
          DESTINATION share/appdata)

  # GNOME Thumbnailer
  install(FILES mime/aseprite.xml
          DESTINATION share/mime/packages)
  install(PROGRAMS libresprite-thumbnailer
          DESTINATION bin)
  install(FILES gnome/libresprite.thumbnailer
          DESTINATION share/thumbnailers)

  # Qt Thumbnailer
  if(WITH_QT_THUMBNAILER)
    add_subdirectory(kde)
  endif()
endif()
