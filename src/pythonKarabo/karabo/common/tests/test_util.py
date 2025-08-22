# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.api import WeakMethodRef
from karabo.common.utils import get_arrowhead_points


def test_weak_method_ref():
    """Test that we can safely call bound methods"""
    global called
    called = False

    class Device:
        def move(self):
            global called
            called = True

    device = Device()
    func = WeakMethodRef(device.move)
    func()
    assert called is True
    called = False
    del device
    func()
    assert called is False


def test_get_arrowhead_points():
    # Test case 1: Horizontal line
    x1, y1, x2, y2 = 0, 0, 100, 0
    hx1, hy1, hx2, hy2 = get_arrowhead_points(x1, y1, x2, y2)

    assert hx1 < x2 and hx2 < x2, (
        "Arrowhead points should be behind the endpoint")
    assert hx1 == hx2, (
        "Arrowhead points should have the same x-coordinate")
    assert hy1 == - hy2, "Left y should be negative of right y "

    # Test case 2: Vertical line
    x1, y1, x2, y2 = 0, 0, 0, 100
    hx1, hy1, hx2, hy2 = get_arrowhead_points(x1, y1, x2, y2)
    assert hy1 < y2 and hy2 < y2, (
        "Arrowhead points should be above the endpoint")
    assert round(hx1, 9) == round(-hx2, 9), (
        "left x should be negative right x")
    assert hy1 == hy2

    # Test case 3: Diagonal line
    x1, y1, x2, y2 = 0, 0, 100, 100
    hx1, hy1, hx2, hy2 = get_arrowhead_points(x1, y1, x2, y2)

    assert not hx1 == hx2, "Arrowhead points X-coordinates should be distinct"
    assert not hy1 == hy2, "Arrowhead points Y-coordinates should be distinct"
    assert hx1 < x2 and hx2 < x2, (
        "Arrowhead points should be behind the endpoint")
    assert hy1 < y2 and hy2 < y2, (
        "Arrowhead points should be below the endpoint")

    # Test case 4: Negative coordinates
    x1, y1, x2, y2 = -100, -100, -200, -200
    hx1, hy1, hx2, hy2 = get_arrowhead_points(x1, y1, x2, y2)

    assert not hx1 == hx2, "Arrowhead points X-coordinates should be distinct"
    assert not hy1 == hy2, "Arrowhead points Y-coordinates should be distinct"
    assert hx1 == hy2
    assert hy1 == hx2
