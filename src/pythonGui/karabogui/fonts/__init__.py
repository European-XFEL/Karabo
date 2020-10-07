from pathlib import Path

from PyQt5.QtGui import QFont

from karabo.common.scenemodel.api import SCENE_FONT_FAMILY


def get_font_from_string(font_string):
    q_font = QFont()
    q_font.fromString(font_string)

    # Compare style hint and replace accordingly
    if q_font.family() not in FONT_FAMILIES:
        font = FONT_STYLE_HINTS.get(q_font.styleHint(), SCENE_FONT_FAMILY)
        q_font.setFamily(font)

    return q_font


def substitute_font(model):
    if "font" not in model.trait_names():
        return

    q_font = get_font_from_string(model.font)
    model.trait_setq(font=q_font.toString())


def _get_font_filenames():
    # Returns the list of font filenames relative to the fonts/ folder.
    # This is then used when adding the fonts in the font database."""
    folder = Path(__file__).parent
    return [str(filename) for filename in folder.rglob("*.ttf")]


FONT_FILENAMES = _get_font_filenames()

# This maps the font family to a Qt style hint to ease font replacement
FONT_STYLE_HINTS = {
    QFont.AnyStyle: "Source Sans Pro",
    QFont.Helvetica: "Source Sans Pro",
    QFont.Times: "Source Serif Pro",
    QFont.Courier: "Source Code Pro"
}

FONT_FAMILIES = set(FONT_STYLE_HINTS.values())
