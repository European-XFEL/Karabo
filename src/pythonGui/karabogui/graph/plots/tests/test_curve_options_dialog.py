import pytest
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialogButtonBox, QMessageBox

from karabogui.graph.plots.api import CurveOptionsDialog
from karabogui.testing import click_button

PINK = "#fb9a99"
ORANGE = "#ff7f00"

curve1 = object()
curve2 = object()

OPTIONS = {curve1: {"key": "first_plot", "pen_color": ORANGE,
                    "name": "Curve 1", "curve_type": 1},
           curve2: {"key": "second_plot", "pen_color": PINK,
                    "name": "Curve 2", "curve_type": 1}}


@pytest.fixture
def dialog(gui_app):
    dialog = CurveOptionsDialog(curve_options=OPTIONS)
    yield dialog


def test_load_options(dialog):
    proxy_list = dialog.proxy_list
    assert proxy_list.count() == len(OPTIONS)
    assert proxy_list.item(0).text() == "first_plot"
    assert proxy_list.item(0).data(Qt.UserRole) == OPTIONS[curve1]
    assert proxy_list.item(0).data(Qt.UserRole + 1) == curve1

    assert proxy_list.item(1).text() == "second_plot"
    assert proxy_list.item(1).data(Qt.UserRole) == OPTIONS[curve2]
    assert proxy_list.item(1).data(Qt.UserRole + 1) == curve2


def test_color_combobox(dialog):
    assert dialog.color_combo_box.count() == 13
    assert dialog.color_combo_box.itemText(0) == ""
    assert dialog.color_combo_box.itemText(11) == ""


def test_displayed_options(dialog):
    dialog.proxy_list.setCurrentRow(0)
    dialog.color_combo_box.currentText() == ""
    dialog.legend_name.text() == "Curve 1"
    dialog.property_key.text() == "first_plot"

    dialog.proxy_list.setCurrentRow(1)
    dialog.color_combo_box.currentText() == ""
    dialog.legend_name.text() == "Curve 2"
    dialog.property_key.text() == "second_plot"


def test_get_curve_options(dialog):
    # No changes yet
    assert dialog.get_curve_options() == {}
    assert dialog._original == OPTIONS

    # Change option values.
    dialog.proxy_list.setCurrentRow(1)
    dialog.legend_name.setText("New Legend")
    dialog.legend_name.editingFinished.emit()
    dialog.color_combo_box.setCurrentText("Orange")
    curve_options = dialog.get_curve_options()
    assert curve_options[curve1] == OPTIONS[curve1]

    assert curve_options[curve2]["pen_color"] == PINK
    assert curve_options[curve2]["name"] == "New Legend"


def test_get_curve_options_reset(dialog, mocker):
    path = "karabogui.graph.plots.dialogs.curve_options.QMessageBox"
    message_box = mocker.patch(path)
    message_box.return_value = QMessageBox.Yes
    click_button(dialog.buttonBox.button(QDialogButtonBox.Reset))
    assert dialog.has_reset
    assert dialog.get_curve_options() is None
