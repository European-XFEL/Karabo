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


def set_roi_html(name, center=None, size=None):
    html_list = []

    # Name
    html_list.append(
        f'<span style="color: #FFF; font-size: 8pt; font-weight: bold;">'
        f'{name or "Region of Interest"}</span>')
    # Center
    if center is not None:
        html_list.append(
            f'<span style="color: #FFF; font-size: 8pt;">'
            f'Center: ({center[0]}, {center[1]})</span>')
    # Size
    if size is not None:
        html_list.append(
            f'<span style="color: #FFF; font-size: 8pt;">'
            f'Size: ({size[0]}, {size[1]})</span>')

    html = "<br>".join(html_list)
    return f'<div style="text-align: center">{html}</div>'


class ImageRegion:
    Point = 0
    Line = 1
    Area = 2

    def __init__(self, region=None, region_type=None,
                 x_slice=None, y_slice=None):

        if region is None:
            region = np.empty((0, 0))
        if x_slice is None:
            x_slice = np.empty(0)
        if y_slice is None:
            y_slice = np.empty(0)

        self.region = region
        self.region_type = region_type
        self.slices = [x_slice, y_slice]

    def valid(self, axis=None):
        """Checks if region (all axis) is valid"""
        if self.region_type is ImageRegion.Area:
            # Region is a matrix, so we only check the shape
            return all(shape > 1 for shape in self.region.shape)

        if self.region_type is ImageRegion.Line:
            # Region is a list of two lists
            samples = (0, 1) if axis is None else (axis,)
            return all(self.region[ax].size > 1 for ax in samples)

        return False

    def flatten(self):
        if self.region_type is ImageRegion.Area:
            # region is a numpy array
            return self.region.flatten()

        if self.region_type is ImageRegion.Line:
            # region is a list of 2 lists (x and y most probably have different
            # shapes)
            return np.hstack(self.region)
