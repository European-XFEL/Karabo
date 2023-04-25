# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
