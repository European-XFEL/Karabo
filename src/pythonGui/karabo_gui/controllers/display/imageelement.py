#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QImage, QLabel, QPixmap
from traits.api import on_trait_change, Instance

from karabo.common.scenemodel.api import DisplayImageElementModel
from karabo_gui.binding.api import (
    BaseBindingController, ImageBinding, register_binding_controller
)
from karabo_gui.controllers.images import (
    get_image_data, get_dimensions_and_format)


COLOR_TABLE = [QColor(i, i, i).rgb() for i in range(256)]
DEFAULT_SIZE = 125


@register_binding_controller(ui_name='Image Element', read_only=True,
                             binding_type=ImageBinding)
class DisplayImageElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(DisplayImageElementModel)

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setAutoFillBackground(True)
        widget.setAlignment(Qt.AlignCenter)
        widget.setFixedHeight(DEFAULT_SIZE)
        widget.setMinimumWidth(DEFAULT_SIZE)
        widget.setWordWrap(True)
        return widget

    @on_trait_change('proxies.binding.config_update')
    def _update_image(self, obj, name, value):
        if name != 'config_update':
            return
        if self.widget is None:
            return

        img_node = obj.value

        dimX, dimY, dimZ, format = get_dimensions_and_format(img_node)
        img_types = (QImage.Format_Indexed8, QImage.Format_RGB888)
        if not all((dimX, dimY, format in img_types)):
            return

        npy = get_image_data(img_node, dimX, dimY, dimZ, format)
        if npy is None:
            return

        # Cast
        npy = npy.astype(np.uint8)

        if format is QImage.Format_Indexed8:
            image = QImage(npy.data, dimX, dimY, dimX, format)
            image.setColorTable(COLOR_TABLE)
        elif format is QImage.Format_RGB888:
            image = QImage(npy.data, dimX, dimY, dimX * dimZ, format)
        else:
            return
        pixmap = QPixmap.fromImage(image)

        # Scale pixmap
        if pixmap.height() > DEFAULT_SIZE:
            pixmap = pixmap.scaledToHeight(DEFAULT_SIZE)

        self.widget.setPixmap(pixmap)
