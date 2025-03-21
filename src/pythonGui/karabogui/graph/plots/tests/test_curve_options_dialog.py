import pytest
from qtpy.QtCore import Qt

from karabogui.graph.plots.api import CurveOptionsDialog

PINK = "#fb9a99"
ORANGE = "#ff7f00"

OPTIONS = {"first_plot": {"key": "first_plot", "pen_color": ORANGE,
                          "legend_name": "Curve 1", "plot_type": 1},
           "second_plot": {"key": "second_plot", "pen_color": PINK,
                           "legend_name": "Curve 2", "plot_type": 1}
           }


@pytest.fixture
def dialog(gui_app):
    dialog = CurveOptionsDialog(curve_options=OPTIONS)
    yield dialog


def test_load_options(dialog):
    proxy_list = dialog.proxy_list
    assert proxy_list.count() == len(OPTIONS)
    assert proxy_list.item(0).text() == "first_plot"
    assert proxy_list.item(0).data(Qt.UserRole) == OPTIONS["first_plot"]
    assert proxy_list.item(1).text() == "second_plot"
    assert proxy_list.item(1).data(Qt.UserRole) == OPTIONS["second_plot"]


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
    assert dialog.get_curve_options() == OPTIONS

    # Change option values.
    dialog.proxy_list.setCurrentRow(1)
    dialog.legend_name.setText("New Legend")
    dialog.legend_name.editingFinished.emit()
    dialog.color_combo_box.setCurrentText("Orange")
    curve_options = dialog.get_curve_options()
    assert curve_options["first_plot"] == OPTIONS["first_plot"]

    assert curve_options["second_plot"]["pen_color"] == PINK
    assert curve_options["second_plot"]["legend_name"] == "New Legend"
