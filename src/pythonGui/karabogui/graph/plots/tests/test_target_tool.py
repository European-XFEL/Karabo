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
from pyqtgraph import PlotItem

from karabogui.graph.plots.base import KaraboPlotView
from karabogui.graph.plots.tools import CrossTargetController

X_ARRAY = np.arange(10)
Y_ARRAY = X_ARRAY ** 2


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


def test_legend_color(gui_app):
    """Test the Target tool legend color is black"""
    widget = KaraboPlotView()
    plot = widget.add_curve_item()
    plot.setData(X_ARRAY, Y_ARRAY)
    plot_item = widget.plotItem

    target_tool = CrossTargetController(plot_item)
    target_tool.activate()
    legend = target_tool.legend
    legend_color = legend._label.opts.get("color")
    assert legend_color == "k"
