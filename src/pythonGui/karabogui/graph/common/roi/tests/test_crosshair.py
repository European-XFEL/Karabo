# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pyqtgraph import Point

from karabogui.graph.common.api import CrosshairROI


def test_crosshair_positions(gui_app):
    crosshair = CrosshairROI(pos=Point(3.2, 2.1))
    assert crosshair.coords == (3.2, 2.1)
    assert crosshair.pos() == Point(3.2, 2.1)
    assert crosshair.getSnapPosition(6.9, 7.4) == Point(6.0, 6.0)
