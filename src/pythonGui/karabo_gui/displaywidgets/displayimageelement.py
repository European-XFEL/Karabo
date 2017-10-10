#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QImage, QLabel, QPixmap

from karabo_gui.images import get_image_data, get_dimensions_and_format
from karabo_gui.schema import ImageNode
from karabo_gui.widget import DisplayWidget


class DisplayImageElement(DisplayWidget):
    category = ImageNode
    alias = "Image Element"

    colorTable = [QColor(i, i, i).rgb() for i in range(256)]

    def __init__(self, box, parent):
        super(DisplayImageElement, self).__init__(box)

        self.image = QLabel(parent)
        self.image.setAutoFillBackground(True)
        self.image.setAlignment(Qt.AlignCenter)
        self.image.setMinimumHeight(125)
        self.image.setMaximumHeight(125)
        self.image.setMinimumWidth(125)
        self.image.setWordWrap(True)

        self.value = None

    @property
    def widget(self):
        return self.image

    def valueChanged(self, box, value, timestamp=None):
        if self.value is not None or value == self.value:
            return

        dimX, dimY, dimZ, format = get_dimensions_and_format(value)
        if dimX is None or dimY is None or (dimZ is not None and dimZ != 3):
            return

        npy = get_image_data(value, dimX, dimY, dimZ, format)
        if npy is None:
            return

        # Cast
        npy = npy.astype(np.uint8)

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        if format is QImage.Format_Indexed8:
            image = QImage(npy.data, dimX, dimY, dimX, format)
            image.setColorTable(self.colorTable)
        elif format is QImage.Format_RGB888:
            image = QImage(npy.data, dimX, dimY, dimX * dimZ, format)
        else:
            return
        pixmap = QPixmap.fromImage(image)

        # Scale pixmap
        if pixmap.height() > 125:
            pixmap = pixmap.scaledToHeight(125)

        self.image.setPixmap(pixmap)
