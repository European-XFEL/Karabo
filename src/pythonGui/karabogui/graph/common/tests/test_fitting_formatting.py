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

from karabogui.graph.common.fitting import gaussian_fit
from karabogui.graph.common.formatting import (
    table_body, table_header, table_row)


def test_formatting():
    assert table_header() == "<th align='left' width='50'>&nbsp;</th>"
    assert table_body() == "<th align='left' width='50'>&nbsp;</th>"
    assert table_row() == "<tr></tr>"

    expected = "<th align='left' width='50'>Gaussian Fit Result</th>"
    assert table_header(header="Gaussian Fit Result") == expected

    expected = ("<th align='left' width='50'>Gaussian Fit Result</th><th "
                "align='left' width='50'>one</th><th align='left' "
                "width='50'>two</th>")
    assert table_header(
        header="Gaussian Fit Result", tabs=("one", "two")) == expected

    expected = ("<th align='left' width='50'>one</th><td "
                "width='50'>t</td><td width='50'>w</td><td width='50'>o</td>")
    assert table_body("one", "two") == expected


def test_gaussian_fitting():
    x = np.array([1, 2, 3, 4, 5])
    h = 1.0
    x0 = 1.0
    sigma = 1.0
    offset = 1.0
    params = gaussian_fit(x, h, x0, sigma, offset)
    expected = np.array([2, 1.60653066, 1.13533528, 1.011109, 1.00033546])
    params = np.round(params, 5)
    expected = np.round(expected, 5)
    assert np.array_equal(params, expected)
