#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 14, 2019
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import os.path as op

from qtpy.QtWidgets import QAction
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ErrorBoolModel
from karabogui import icons
from karabogui.binding.api import BoolBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.widgets.hints import SvgWidget

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
        widget = SvgWidget(parent)
        widget.setMinimumSize(24, 24)

        logicAction = QAction("Invert color logic", widget)
        logicAction.triggered.connect(self.logic_action)
        widget.addAction(logicAction)
        logicAction.setCheckable(True)
        logicAction.setChecked(self.model.invert)

        return widget

    def add_proxy(self, proxy):
        self._update_widget(proxy)
        return True

    def _update_widget(self, proxy=None):
        values = [get_binding_value(p, False) for p in self.proxies]
        if proxy is not None:
            values.append(get_binding_value(proxy, False))

        value = all(values)
        if not self.model.invert:
            svg_file = OK_BOOL if value else ERROR_BOOL
        else:
            svg_file = ERROR_BOOL if value else OK_BOOL
        self.widget.setToolTip("{}".format(value))
        self.widget.load(svg_file)

    def value_update(self, proxy):
        self._update_widget(proxy)

    @on_trait_change('model.invert', post_init=True)
    def _invert_update(self):
        if self.proxy is not None:
            self.value_update(self.proxy)

    def logic_action(self):
        self.model.invert = not self.model.invert
