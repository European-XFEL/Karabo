#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 31, 2022
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
#############################################################################

from qtpy.QtCore import QSize, Qt
from qtpy.QtWidgets import (
    QLabel, QSizePolicy, QStackedLayout, QVBoxLayout, QWidget)

from karabogui.controllers.api import get_model_controller
from karabogui.indicators import get_device_status_pixmap
from karabogui.sceneview.api import get_proxy

from .base import BasePanelWidget

QPADDING = 20


class WidgetControllerPanel(BasePanelWidget):

    def __init__(self, title, model):
        self.model = model
        self._setup_model_controller(model)
        super().__init__(title=title, allow_closing=True)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)

    def sizeHint(self):
        """Provide an appropriate `sizeHint` for this widget controller panel
        """
        width = self.model.width + QPADDING
        height = self.toolbar.height() + self.model.height + QPADDING
        return QSize(width, height)

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
        is_editable = False
        if self.model.parent_component == "EditableApplyLaterComponent":
            is_editable = True
        self.controller.set_read_only(not is_editable)
        self.controller.binding_update(self.proxy)
        self.controller.finish_initialization()
        self.controller.show()

        padding = QPADDING // 2
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(padding, padding, padding, padding)

        self.status_symbol = QLabel("", self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)

        stack_layout = QStackedLayout()
        stack_layout.setStackingMode(QStackedLayout.StackAll)
        stack_layout.addWidget(self.controller.widget)
        stack_layout.addWidget(self.status_symbol)
        layout.addLayout(stack_layout)

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

    def __repr__(self):
        return f"<WidgetControllerPanel property={self.controller.proxy.key}>"
