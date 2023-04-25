# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pyqtgraph import LabelItem

from karabogui.graph.common.api import KaraboLegend, float_to_string


class ScaleLegend(KaraboLegend):

    def __init__(self):
        super(ScaleLegend, self).__init__()
        self._label = LabelItem(color='w', size="8pt")
        self.layout.addItem(self._label, 0, 0)
        self.layout.setContentsMargins(2, 2, 2, 2)

    def set_value(self, x_scale, y_scale, x_units, y_units):
        x_scale, y_scale = [float_to_string(num) for num in [x_scale, y_scale]]
        self._label.setText("<b>Scale</b><br>"
                            "x: {} {}<br>"
                            "y: {} {}".format(x_scale, x_units,
                                              y_scale, y_units))
