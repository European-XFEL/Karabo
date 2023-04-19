#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import os.path as op

from qtpy.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui import icons
from karabogui.binding.api import BoolBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

ICONS = op.dirname(icons.__file__)
CHECKED = op.join(ICONS, "checkbox-checked.svg")
UNCHECKED = op.join(ICONS, "checkbox-unchecked.svg")


@register_binding_controller(ui_name='Toggle Field',
                             klassname='DisplayCheckBox',
                             binding_type=BoolBinding, priority=10)
class DisplayCheckBox(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(CheckBoxModel, args=())

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        widget.setFixedSize(20, 20)
        return widget

    def value_update(self, proxy):
        svg_file = CHECKED if proxy.value else UNCHECKED
        self.widget.load(svg_file)
