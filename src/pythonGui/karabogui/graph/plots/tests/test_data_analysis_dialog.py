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

from karabo.native import Configurable, VectorFloat
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog
from karabogui.testing import get_class_property_proxy

config = {'half_samples': 6000, 'roi_items': [], 'roi_tool': 0, 'offset': 0.0,
          'step': 1.0, 'x_grid': True, 'y_grid': True, 'title': '',
          'background': 'transparent', 'x_label': '',
          'y_label': 'vectorProperty', 'x_units': '', 'y_units': '',
          'x_autorange': True, 'y_autorange': True, 'x_log': False,
          'y_log': False, 'x_invert': False, 'y_invert': False, 'x_min': 0.0,
          'x_max': 0.0, 'y_min': 0.0, 'y_max': 0.0, 'layout_data': None}


class Object(Configurable):
    prop = VectorFloat(defaultValue=[1.1, 2.0, 3.2, 4.5])
    prop2 = VectorFloat()


def test_fetch_data(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0]
    proxy.value = value

    dialog = DataAnalysisDialog(
        proxies=[proxy], config=config, parent=None)

    # Initial values
    combo = dialog.fit_options_combobox
    items = [combo.itemText(i) for i in range(combo.count())]
    expected = ["Gaussian", "Step function (CDF)", "Linear function"]
    assert items == expected
    assert not dialog.auto_update_checkbox.isChecked()
    x, y = dialog.data_curve.getData()
    assert all(y == np.array(value))
    assert dialog.result_label.text() == ""

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
        proxies=[proxy, additional_proxy], config=config, parent=None)
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
