#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 31, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QSize
from qtpy.QtWidgets import QHBoxLayout, QSizePolicy, QWidget

from karabogui.controllers.api import get_model_controller
from karabogui.sceneview.api import get_proxy

from .base import BasePanelWidget


class WidgetControllerPanel(BasePanelWidget):

    def __init__(self, title, model):
        self.model = model
        self._setup_model_controller(model)
        super().__init__(title=title, allow_closing=True)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)

    def sizeHint(self):
        """Provide an appropriate `sizeHint` for this widget controller panel
        """
        height = self.toolbar.height() + self.model.height
        return QSize(self.model.width, height)

    def _setup_model_controller(self, model):
        """Setup the proxy and widget controller for this panels"""
        proxies = [get_proxy(*key.split(".", 1)) for key in self.model.keys]
        self.proxy = proxies[0]
        controller = get_model_controller(model)(
            proxy=self.proxy, model=model)
        for proxy in proxies[1:]:
            controller.visualize_additional_property(proxy)
        self.controller = controller

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel"""
        widget = QWidget(self)
        self.controller.create(self)
        self.controller.set_read_only(False)
        self.controller.binding_update(self.proxy)
        self.controller.finish_initialization()
        self.controller.show()

        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)
        widget.setLayout(layout)

        return widget

    def closeEvent(self, event):
        super().closeEvent(event)
        if event.isAccepted():
            self.signalPanelClosed.emit(self.windowTitle())
            self.controller.destroy()

    def onUndock(self):
        super().onUndock()
        self.resize(self.sizeHint())
