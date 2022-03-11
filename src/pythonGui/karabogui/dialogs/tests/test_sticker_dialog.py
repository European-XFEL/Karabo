from unittest import mock

from qtpy.QtGui import QColor, QFont
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import StickerModel
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.api import StickerDialog
from karabogui.testing import click_button, gui_app


def test_sticker_dialog(gui_app: gui_app):

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

    psize = 10 if not IS_MAC_SYSTEM else 13
    text = f"Source Sans Pro,{psize},-1,5,50,0,0,0,0,0"
    assert dialog.text_font.toString() == text

    # Only the internal dialog model is modified
    text_widget.setPlainText("XFEL2")
    assert model.text == "XFEL"
    assert dialog.model.text == "XFEL2"

    path = "karabogui.dialogs.sticker_dialog.FontDialog"
    with mock.patch(path) as d:
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
    with mock.patch(path) as d:
        c = QColor(255, 0, 0)
        d.getColor.return_value = c
        click_button(dialog.pbBackground)
        assert dialog.model.background == c.name()
        assert dialog.model.background != "transparent"

    # Change the foreground
    assert dialog.model.foreground == ""
    path = "karabogui.dialogs.sticker_dialog.QColorDialog"
    with mock.patch(path) as d:
        c = QColor(0, 255, 0)
        d.getColor.return_value = c
        click_button(dialog.pbTextColor)
        assert dialog.model.foreground == c.name()
