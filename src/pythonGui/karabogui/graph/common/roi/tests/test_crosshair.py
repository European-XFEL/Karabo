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
from pyqtgraph import Point

from karabogui.graph.common.api import CrosshairROI


def test_crosshair_positions(gui_app):
    crosshair = CrosshairROI(pos=Point(3.2, 2.1))
    assert crosshair.coords == (3.2, 2.1)
    assert crosshair.pos() == Point(3.2, 2.1)
    assert crosshair.getSnapPosition(6.9, 7.4) == Point(6.0, 6.0)
