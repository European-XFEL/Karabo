#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 5, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from pathlib import Path

from qtpy.QtCore import QSize
from qtpy.QtGui import QPixmap
from qtpy.QtSvg import QSvgRenderer, QSvgWidget
from qtpy.QtWidgets import QHBoxLayout, QLabel, QWidget

from karabo.common.scenemodel.api import convert_from_svg_image
from karabogui import icons
from karabogui.widgets.hints import KaraboSceneWidget

MISSING_ICON = str(Path(icons.__file__).parent / "image-missing.svg")


class ImageRendererWidget(KaraboSceneWidget, QWidget):

    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        layout = QHBoxLayout()
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)
        image_format, data = convert_from_svg_image(model.image)
        if image_format == "svg":
            widget = QSvgWidget(self)
            renderer = QSvgRenderer(data)
            if not renderer.isValid() or renderer.defaultSize().isNull():
                data = MISSING_ICON
            widget.load(data)
        else:
            widget = QLabel(self)
            widget.setScaledContents(True)
            pixmap = QPixmap()
            pixmap.loadFromData(data)
            if pixmap.isNull():
                pixmap = icons.imageMissing.pixmap(QSize(50, 50))
            widget.setPixmap(pixmap)

        self._internal_widget = widget
        self.layout().addWidget(widget)
        self.setGeometry(model.x, model.y, model.width, model.height)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""
        self._internal_widget.destroy()
        super().destroy()

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)