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
import numpy as np
import pytest
from numpy.testing import assert_array_equal

from karabo.native import Configurable, VectorFloat
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog
from karabogui.testing import get_class_property_proxy

CONFIG = {'half_samples': 6000, 'roi_items': [], 'roi_tool': 0, 'offset': 0.0,
          'step': 1.0, 'x_grid': True, 'y_grid': True, 'title': '',
          'background': 'transparent', 'x_label': '',
          'y_label': 'vectorProperty', 'x_units': '', 'y_units': '',
          'x_autorange': True, 'y_autorange': True, 'x_log': False,
          'y_log': False, 'x_invert': False, 'y_invert': False, 'x_min': 0.0,
          'x_max': 0.0, 'y_min': 0.0, 'y_max': 0.0, 'layout_data': None}


class Object(Configurable):
    prop = VectorFloat(defaultValue=[1.1, 2.0, 3.2, 4.5])
    prop2 = VectorFloat()
    baseline = VectorFloat()


def test_fetch_data(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0]
    proxy.value = value

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=CONFIG, parent=None)

    # Initial values
    combo = dialog.fit_options_combobox
    items = [combo.itemText(i) for i in range(combo.count())]
    expected = ["Gaussian", "Sech Square", "Step function (CDF)",
                "Linear function"]
    assert items == expected
    assert not dialog.fit_on_update_checkbox.isChecked()
    x, y = dialog.data_curve.getData()
    assert all(y == np.array(value))
    assert dialog.result_label.text() == ""
    assert list(dialog.x_values) == list(range(len(proxy.value)))
    assert_array_equal(dialog.y_values, proxy.value)

    # Update from the parent plot.
    value = [10, 20, 30]
    proxy.value = value
    dialog.update_data()
    x, y = dialog.data_curve.getData()
    assert all(y == np.array(value))


def test_multiple_proxies(gui_app, mocker):
    """
    Test dialog when the plot has multiple proxies.
    """
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0]
    proxy.value = value

    additional_proxy = get_class_property_proxy(schema, "prop2")
    additional_proxy.value = [0.1, 0.2, 0.3, 0.4]

    dialog = DataAnalysisDialog(
        proxies=[proxy, additional_proxy], config=CONFIG, parent=None)
    combo_box = dialog.property_comboBox
    assert combo_box.currentText() == proxy.key
    assert combo_box.itemText(0) == proxy.key
    assert combo_box.itemText(1) == additional_proxy.key

    # When combobox item is changed, the plot should get updated.
    method = mocker.patch.object(dialog, "update_data")
    assert method.call_count == 0
    combo_box.setCurrentIndex(1)
    assert dialog.proxy == combo_box.currentData()
    assert dialog.proxy == additional_proxy
    assert method.call_count == 1


@pytest.mark.filterwarnings("ignore::scipy.optimize.OptimizeWarning")
def test_sub_region_roi(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0]
    proxy.value = value

    additional_proxy = get_class_property_proxy(schema, "prop2")
    additional_proxy.value = [0.1, 0.2, 0.3, 0.4]

    dialog = DataAnalysisDialog(
        proxies=[proxy, additional_proxy], config=CONFIG, parent=None)
    # Before fitting
    assert not dialog.show_line_roi_button.isChecked()
    assert dialog.left_line
    assert dialog.right_line
    assert not dialog.left_line.isVisible()
    assert not dialog.right_line.isVisible()

    assert dialog.fit_curve.getData() == (None, None)
    assert dialog._min_width == 1.0

    # Fit with no line roi defined.
    dialog.fit()
    _, y_values = dialog.fit_curve.getData()
    assert len(y_values) == 7

    # With line roi
    dialog.show_line_roi_button.click()
    assert dialog.show_line_roi_button.isChecked()
    assert dialog.left_line.isVisible()
    assert dialog.right_line.isVisible()
    assert dialog.left_line.pos().x() == 0
    assert dialog.right_line.pos().x() == 6.0

    # With whole plot selected
    dialog.fit()
    _, y_values = dialog.fit_curve.getData()
    assert len(y_values) == 6

    # With only a region selected
    dialog.left_line.setPos(1.0)
    dialog.right_line.setPos(5.0)
    dialog.fit()
    _, y_values = dialog.fit_curve.getData()
    assert len(y_values) == 4

    # Change property should update the lines position
    dialog.property_comboBox.setCurrentIndex(1)
    assert dialog.property_comboBox.currentText() == "prop2"
    assert dialog.left_line.pos().x() == 0
    assert dialog.right_line.pos().x() == 3.0
    assert dialog.left_pos_line_edit.text() == "0"
    assert dialog.right_pos_line_edit.text() == "3"


def test_sub_region_value_type(gui_app):
    """ Test that no traceback with Linear fit with selection"""
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0]
    proxy.value = value

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=CONFIG, parent=None)
    fit_option = dialog.fit_options_combobox
    fit_option.setCurrentIndex(3)
    assert fit_option.currentText() == "Linear function"
    assert not dialog.show_line_roi_button.isChecked()
    dialog.fit_button.click()

    dialog.show_line_roi_button.setChecked(True)
    dialog.fit_button.click()


def test_baseline_proxy(gui_app):
    """Make sure the X-axis values are from baseline_proxy"""
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0]
    proxy.value = value

    baseline = get_class_property_proxy(schema, "baseline")
    baseline.value = [70, 60, 50, 40, 30, 20, 10]

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=CONFIG, baseline_proxy=baseline)
    assert_array_equal(dialog.x_values, baseline.value)
    assert_array_equal(dialog.y_values, proxy.value)

    # Make sure the config options are affected only when generating
    # baseline and not with baseline_proxy.
    config = CONFIG.copy()
    config["step"] = 1.5
    config["offset"] = 3.0
    dialog = DataAnalysisDialog(
        proxies=[proxy], config=CONFIG, baseline_proxy=baseline)
    assert_array_equal(dialog.x_values, baseline.value)
    assert_array_equal(dialog.y_values, proxy.value)

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=config, baseline_proxy=None)
    expected = np.array([3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12])
    assert_array_equal(dialog.x_values, expected)

    assert np.isclose(dialog._min_width, 1.425, rtol=0.005)


def test_sech_sequred(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 5.0, 7.0, 9.0, 10.0, 8.0, 6.0, 4.0]
    proxy.value = value

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=CONFIG, parent=None)
    fit_option = dialog.fit_options_combobox
    fit_option.setCurrentIndex(1)
    assert fit_option.currentText() == "Sech Square"
    dialog.fit()
    fit_x, fit_y = dialog.fit_curve.getData()
    expected = np.array([0.98434883, 1.76325783, 3.04367143, 4.930586,
                        7.21354721, 9.10750318, 9.56312261, 8.27814066,
                        6.0563046, 3.91234989])
    np.isclose(expected, fit_y)


def test_auto_update(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 5.0, 7.0, 9.0, 10.0, 8.0, 6.0, 4.0]
    proxy.value = value
    dialog = DataAnalysisDialog(proxies=[proxy], config=CONFIG, parent=None)
    x, y = dialog.data_curve.getData()
    assert_array_equal(y, value)

    # Plot should not be updated on proxy update.
    dialog.proxy.binding.config_update = True
    new_value = np.array([60, 50, 40, 30, 20, 10])
    proxy.value = new_value
    x, y = dialog.data_curve.getData()
    with np.testing.assert_raises(AssertionError):
        assert_array_equal(y, new_value)

    # With auto_update checked, plot should be updated on proxy update.
    dialog.auto_update.setChecked(True)
    dialog.proxy.binding.config_update = True
    dialog.proxy.value = new_value
    x, y = dialog.data_curve.getData()
    assert_array_equal(y, new_value)
