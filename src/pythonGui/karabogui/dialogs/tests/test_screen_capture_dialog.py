import pytest
from qtpy.QtWidgets import QApplication

from karabogui.dialogs.screen_capture_dialog import ScreenCaptureDialog


@pytest.fixture
def screen_capture_dialog(gui_app):
    dialog = ScreenCaptureDialog()
    return dialog


def test_inital_setup(screen_capture_dialog):
    assert (len(QApplication.screens()) ==
            screen_capture_dialog.screen_combobox.count())
    assert screen_capture_dialog.info() is None


def test_accept_dialog(screen_capture_dialog, mocker):
    logbook_preview_mock = mocker.patch(
        "karabogui.dialogs.screen_capture_dialog.LogBookPreview")
    screen_capture_dialog.accept()
    assert logbook_preview_mock().show.call_count == 1
