from pyqtgraph import PlotItem

from karabogui.graph.plots.tools import CrossTargetController


def test_logMode(gui_app):
    plot_item = PlotItem()
    controller = CrossTargetController(plot_item)
    assert controller._get_x_value(0.0457) == "0.046"
    assert controller._get_y_value(1) == "1"
    assert controller._get_y_value(2) == "2"
    assert controller._get_y_value(2.823) == "2.823"

    plot_item.setLogMode(x=True, y=True)
    controller = CrossTargetController(plot_item)

    # Value should be in 'e' notation
    assert controller._get_x_value(0.0457) == "1.111e+00"
    assert controller._get_y_value(1) == "1.000e+01"
    assert controller._get_y_value(2) == "1.000e+02"
    assert controller._get_y_value(2.823) == "6.653e+02"
