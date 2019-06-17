#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 14, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4.QtGui import QAction
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ErrorBoolModel
from karabogui import icons
from karabogui.binding.api import BoolBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

ICONS = op.dirname(icons.__file__)
OK_BOOL = op.join(ICONS, "ok-bool.svg")
ERROR_BOOL = op.join(ICONS, "error-bool.svg")


@register_binding_controller(ui_name='Ok-Error Bool',
                             klassname='DisplayErrorBool',
                             binding_type=BoolBinding, priority=0)
class DisplayErrorBool(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ErrorBoolModel, args=())

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        widget.setMinimumSize(24, 24)

        logicAction = QAction("Invert color logic", widget)
        logicAction.triggered.connect(self.logic_action)
        widget.addAction(logicAction)
        logicAction.setCheckable(True)
        logicAction.setChecked(self.model.invert)

        return widget

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if self.widget is None or value is None:
            return

        if not self.model.invert:
            svg = OK_BOOL if value else ERROR_BOOL
        else:
            svg = ERROR_BOOL if value else OK_BOOL
        self.widget.setToolTip("{}".format(value))
        self.widget.load(svg)

    @on_trait_change('model.invert')
    def _invert_update(self):
        if self.proxy is not None:
            self.value_update(self.proxy)

    @pyqtSlot()
    def logic_action(self):
        self.model.invert = not self.model.invert
