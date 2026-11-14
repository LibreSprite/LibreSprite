# dmgbuild settings for the LibreSprite "drag to install" DMG.
#
# Usage:
#   dmgbuild -Dapp=bundle/libresprite.app -s packaging/macos/dmg_settings.py "LibreSprite" libresprite.dmg
#
# `app` defaults to bundle/libresprite.app so this also works when invoked
# without -D from the repo root.

import os.path

application = defines.get('app', 'bundle/libresprite.app')
appname = os.path.basename(application)

format = 'UDZO'
files = [application]
symlinks = {'Applications': '/Applications'}

# Retry `hdiutil detach` a few times: on CI a background process (Spotlight
# mds/mdworker, Finder) can still hold a handle on the freshly mounted image
# when dmgbuild tries to eject it, making the eject fail with "Resource busy".
detach_retries = 5

background = 'packaging/macos/splash.png'

window_rect = ((0, 0), (640, 320))
icon_size = 90
icon_locations = {
    appname: (170, 160),
    'Applications': (470, 160),
}

show_status_bar = False
show_tab_view = False
show_pathbar = False
show_sidebar = False
show_icon_preview = False
default_view = 'icon-view'
include_icon_view_settings = 'auto'
