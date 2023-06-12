# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtGui import QFont

from ..fonts import FONT_STYLE_HINTS, get_qfont

# This maps the font family to a Qt style hint to ease font replacement
SUGGESTED_STYLE_HINTS = {
    QFont.AnyStyle: "Sans Serif",
    QFont.Helvetica: "Helvetica",
    QFont.Times: "Times",
    QFont.Courier: "Courier"
}


def test_get_font_from_string():
    for style, family in FONT_STYLE_HINTS.items():
        # Check if font is loaded
        old_qfont = QFont(family)
        assert old_qfont.family() == family

        # Check if suggested font family is replaced by the application font
        # accordingly
        new_qfont = get_qfont(family)
        assert new_qfont.family() == FONT_STYLE_HINTS[style]
