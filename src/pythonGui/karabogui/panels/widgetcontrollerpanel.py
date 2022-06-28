#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 31, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QSize, Qt
from qtpy.QtWidgets import QLabel, QSizePolicy, QStackedLayout, QWidget

from karabogui.controllers.api import get_model_controller
from karabogui.indicators import get_device_status_pixmap
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

        self.status_symbol = QLabel("", self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)

        layout = QStackedLayout()
        layout.setStackingMode(QStackedLayout.StackAll)
        layout.addWidget(self.controller.widget)
        layout.addWidget(self.status_symbol)
        widget.setLayout(layout)

        proxy = self.controller.proxy.root_proxy
        proxy.on_trait_change(self._proxy_status_changed, "status")
        self._proxy_status_changed(proxy.status)

        return widget

    def _proxy_status_changed(self, value):
        """Show depending on the status if we are online or offline"""
        pixmap = get_device_status_pixmap(value)
        if pixmap is not None:
            self.status_symbol.setPixmap(pixmap)
            self.status_symbol.show()
        else:
            self.status_symbol.hide()

    def closeEvent(self, event):
        super().closeEvent(event)
        if event.isAccepted():
            self.signalPanelClosed.emit(self.windowTitle())
            proxy = self.controller.proxy.root_proxy
            proxy.on_trait_change(self._proxy_status_changed, "status",
                                  remove=True)
            self.controller.destroy()

    def onUndock(self):
        super().onUndock()
        self.resize(self.sizeHint())
