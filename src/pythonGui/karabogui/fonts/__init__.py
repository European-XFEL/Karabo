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
from pathlib import Path

from qtpy.QtGui import QFont, QFontMetrics

from karabo.common.scenemodel.api import SCENE_FONT_FAMILY, SCENE_FONT_SIZE
from karabogui.const import GUI_DPI_FACTOR


def get_qfont(font_string='', adjust_size=True):
    assert type(font_string) is str

    q_font = QFont()
    if font_string:
        q_font.fromString(font_string)
    else:
        # Enforce scene font size. We do not depend on the QApplication font
        # anymore as it scales depending on the OS
        q_font.setPointSize(SCENE_FONT_SIZE)

    # Correct font size for Mac OSX
    if adjust_size:
        font_size = get_font_size_from_dpi(q_font.pointSize())
        q_font.setPointSize(font_size)

    # Compare style hint and replace accordingly
    if q_font.family() not in FONT_FAMILIES:
        font = FONT_STYLE_HINTS.get(q_font.styleHint(), SCENE_FONT_FAMILY)
        q_font.setFamily(font)

    return q_font


def get_font_metrics():
    """Return the FontMetric for the default QFont"""
    q_font = get_qfont()
    return QFontMetrics(q_font)


def substitute_font(model):
    if "font" not in model.trait_names():
        return

    q_font = get_qfont(model.font, adjust_size=False)
    model.trait_setq(font=q_font.toString())


def _get_font_filenames():
    # Returns the list of font filenames relative to the fonts/ folder.
    # This is then used when adding the fonts in the font database."""
    folder = Path(__file__).parent
    return [str(filename) for filename in folder.rglob("*.ttf")]


FONT_FILENAMES = _get_font_filenames()

# This maps the font family to a Qt style hint to ease font replacement
FONT_STYLE_HINTS = {
    QFont.Helvetica: "Source Sans Pro",
    QFont.Times: "Source Serif Pro",
    QFont.Courier: "Source Code Pro"
}

FONT_FAMILIES = sorted(list(set(FONT_STYLE_HINTS.values())))

# This maps the font family to a Qt style hint to ease font replacement
FONT_STYLE_ALIAS = {
    QFont.Helvetica: "Sans Serif",
    QFont.Times: "Serif",
    QFont.Courier: "Monospaced"
}

FONT_ALIAS = sorted(list(set(FONT_STYLE_ALIAS.values())))


def get_font_from_alias(alias):
    for hint, name in FONT_STYLE_ALIAS.items():
        if alias == name:
            return FONT_STYLE_HINTS[hint]

    return SCENE_FONT_FAMILY


def get_alias_from_font(font_family):
    for hint, family in FONT_STYLE_HINTS.items():
        if font_family == family:
            return FONT_STYLE_ALIAS[hint]

    return SCENE_FONT_FAMILY


def get_font_size_from_dpi(size):
    """Calculates the effective font size from the OS DPI.
    As Qt and the majority of the OSes use DPI=96, we should add some
    scaling factor on the sizes for Mac DPI, as it has DPI=72.
    We round off to the nearest integer."""
    return round(size * GUI_DPI_FACTOR)
