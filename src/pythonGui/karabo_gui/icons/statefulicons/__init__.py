from os import path

from .color_change_icon import get_color_change_icons

_icon_path = path.join(path.dirname(__file__),  "iconset")
ICONS = get_color_change_icons(_icon_path)
