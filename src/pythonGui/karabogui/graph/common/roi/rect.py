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
from pyqtgraph import Point
from qtpy.QtCore import QRectF, QSizeF

from .base import KaraboROI
from .utils import ImageRegion


class RectROI(KaraboROI):

    def _add_handles(self):
        self.addScaleHandle([1, 1], [0, 0])

    def get_region(self, imageItem):
        # We only get the region if the ROI is inside the image
        if not imageItem.rect().intersects(self.rect()):
            return ImageRegion()

        # Compute ROI manually
        start_x, start_y = self._absolute_position
        if start_x < 0:
            start_x = 0
        if start_y < 0:
            start_y = 0

        end_x, end_y = self._absolute_position + self._absolute_size

        _, _, max_x, max_y = imageItem.boundingRect().getCoords()
        if end_x > max_x:
            end_x = max_x
        if end_y > max_y:
            end_y = max_y

        x_slice = slice(int(start_x), int(end_x))
        y_slice = slice(int(start_y), int(end_y))

        region = imageItem.image[y_slice, x_slice]

        image_region = ImageRegion(region, ImageRegion.Area, x_slice, y_slice)

        return image_region

    # ---------------------------------------------------------------------
    # PyQt methods

    def setRect(self, rect):
        self.setPos(rect.topLeft(), finish=False)
        self.setSize(rect.size(), finish=False)

    def rect(self):
        return QRectF(self.pos(), QSizeF(*self.size()))

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def getSnapPosition(self, pos, snap=None):
        """Patching to support snapping based on scaling"""
        scaled_pos = np.floor(np.array(pos) / self._scaling) * self._scaling

        return Point(*scaled_pos)
