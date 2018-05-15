#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QFrame, QImage, QLabel, QPixmap
from traits.api import Instance

from karabo.common.scenemodel.api import DisplayImageElementModel
from karabo.middlelayer import EncodingType
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, get_image_data, get_dimensions_and_encoding,
    register_binding_controller)


COLOR_TABLE = [QColor(i, i, i).rgb() for i in range(256)]
DEFAULT_SIZE = 125


@register_binding_controller(ui_name='Image Element',
                             klassname='DisplayImageElement',
                             binding_type=ImageBinding,
                             can_show_nothing=False)
class DisplayImageElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(DisplayImageElementModel, args=())

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setFrameStyle(QFrame.Panel | QFrame.Sunken)
        widget.setAutoFillBackground(True)
        widget.setAlignment(Qt.AlignCenter)
        widget.setFixedHeight(DEFAULT_SIZE)
        widget.setMinimumWidth(DEFAULT_SIZE)
        widget.setWordWrap(True)
        return widget

    def value_update(self, proxy):
        img_node = proxy.value
        dimX, dimY, dimZ, encoding = get_dimensions_and_encoding(img_node)
        npy = get_image_data(img_node, dimX, dimY, dimZ)
        if npy is None:
            return

        # Cast
        npy = npy.astype(np.uint8)

        if encoding == EncodingType.GRAY and not dimZ:
            image = QImage(npy.data, dimX, dimY, dimX,
                           QImage.Format_Indexed8)
            image.setColorTable(COLOR_TABLE)
        elif encoding == EncodingType.RGB and dimZ == 3:
            image = QImage(npy.data, dimX, dimY, dimX * dimZ,
                           QImage.Format_RGB888)
        elif encoding == EncodingType.YUV:
            if dimZ == 2:  # YUV422
                # input image is (u1, y1, v1, y2, u2, y3, v2, y4, ...)
                # only display "luma" (Y') component in the GUI
                npy = np.copy(npy[:, :, 1])
                image = QImage(npy.data, dimX, dimY, dimX,
                               QImage.Format_Indexed8)
            elif dimZ == 3:  # YUV444
                # input image is (u1, y1, v1, u2, y2, v2, ...)
                # only display "luma" (Y') component in the GUI
                npy = np.copy(npy[:, :, 1])
                image = QImage(npy.data, dimX, dimY, dimX,
                               QImage.Format_Indexed8)
            else:
                return
            image.setColorTable(COLOR_TABLE)
        else:
            return
        pixmap = QPixmap.fromImage(image)

        # Scale pixmap
        if pixmap.height() > DEFAULT_SIZE:
            pixmap = pixmap.scaledToHeight(DEFAULT_SIZE)

        self.widget.setPixmap(pixmap)
