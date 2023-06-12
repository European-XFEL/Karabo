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

from qtpy.QtGui import QColor, QFont
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import StickerModel
from karabogui.dialogs.api import StickerDialog
from karabogui.fonts import get_alias_from_font
from karabogui.testing import click_button


def test_sticker_dialog(gui_app, mocker):
    """Test the sticker dialog"""
    model = StickerModel(
        text="XFEL",
        width=100, height=200)
    dialog = StickerDialog(model)

    assert model != dialog.model
    assert dialog.styleSheet() == ""

    text_widget = dialog.leText
    assert text_widget.width() == model.width
    assert text_widget.height() == model.height
    assert text_widget.toPlainText() == model.text
    assert dialog.pbFont.text() == "Sans Serif, 10pt"

    text = "Source Sans Pro,10,-1,5,50,0,0,0,0,0"
    assert dialog.text_font.toString() == text

    # Only the internal dialog model is modified
    text_widget.setPlainText("XFEL2")
    assert model.text == "XFEL"
    assert dialog.model.text == "XFEL2"

    path = "karabogui.dialogs.sticker_dialog.FontDialog"
    d = mocker.patch(path)
    font = QFont()
    font.setPointSize(20)
    d().qfont = font
    d().exec.return_value = QDialog.Accepted
    click_button(dialog.pbFont)
    assert dialog.model.font == font.toString()

    assert dialog.pbBackground.isEnabled() is True
    assert dialog.model.background == "white"
    click_button(dialog.cbBackground)
    assert dialog.pbBackground.isEnabled() is False
    assert dialog.model.background == "transparent"

    # Enable again to change background
    click_button(dialog.cbBackground)
    path = "karabogui.dialogs.sticker_dialog.QColorDialog"
    d = mocker.patch(path)
    c = QColor(255, 0, 0)
    d.getColor.return_value = c
    click_button(dialog.pbBackground)
    assert dialog.model.background == c.name()
    assert dialog.model.background != "transparent"

    # Change the foreground
    assert dialog.model.foreground == ""
    path = "karabogui.dialogs.sticker_dialog.QColorDialog"
    d = mocker.patch(path)
    c = QColor(0, 255, 0)
    d.getColor.return_value = c
    click_button(dialog.pbTextColor)
    assert dialog.model.foreground == c.name()
    assert dialog.pbFont.text() == "Source Sans Pro, 20pt"


def test_set_text_font_button(gui_app):
    """
    The Font buttons text and font family is updated, on changing the font
    selection. Font size remains same.
    """
    model = StickerModel(
        text="XFEL",
        width=100, height=200)
    dialog = StickerDialog(model)

    font = QFont()
    font.setFamily("Monospaced")
    font.setPointSize(23)
    dialog.text_font = font
    dialog.set_text_font_button()
    button_font = dialog.pbFont.font()

    assert button_font.family() == "Monospaced"
    assert button_font.pointSize() == 10
    text = get_alias_from_font("Monospaced") + ", 23pt"
    assert dialog.pbFont.text() == text

    font.setFamily("Sans Serif")
    font.setPointSize(30)
    dialog.text_font = font
    dialog.set_text_font_button()

    button_font = dialog.pbFont.font()
    assert button_font.family() == "Sans Serif"
    assert button_font.pointSize() == 10
    text = get_alias_from_font("Sans Serif") + ", 30pt"
    assert dialog.pbFont.text() == text

    font.setStrikeOut(True)
    font.setBold(True)
    font.setItalic(True)
    dialog.text_font = font
    dialog.set_text_font_button()

    button_font = dialog.pbFont.font()
    assert button_font.strikeOut()
    assert button_font.bold()
    assert button_font.italic()

    font.setStrikeOut(False)
    font.setBold(False)
    font.setItalic(False)
    dialog.text_font = font
    dialog.set_text_font_button()

    button_font = dialog.pbFont.font()
    assert not button_font.strikeOut()
    assert not button_font.bold()
    assert not button_font.italic()
