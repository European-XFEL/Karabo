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
from pyqtgraph import LabelItem

from karabogui.graph.common.api import KaraboLegend, float_to_string


class ScaleLegend(KaraboLegend):

    def __init__(self):
        super().__init__()
        self._label = LabelItem(color='w', size="8pt")
        self.layout.addItem(self._label, 0, 0)
        self.layout.setContentsMargins(2, 2, 2, 2)

    def set_value(self, x_scale, y_scale, x_units, y_units):
        x_scale, y_scale = (float_to_string(num) for num in [x_scale, y_scale])
        self._label.setText("<b>Scale</b><br>"
                            "x: {} {}<br>"
                            "y: {} {}".format(x_scale, x_units,
                                              y_scale, y_units))
