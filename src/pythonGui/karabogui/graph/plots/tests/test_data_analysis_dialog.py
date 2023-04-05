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


def test_fetch_data(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    value = [1.0, 2.0, 3.0, 4.0]
    proxy.value = value

    dialog = DataAnalysisDialog(
        proxy=proxy, config=config, parent=None)

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
