#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui import icons
from karabogui.binding.api import (
    BaseBindingController, BoolBinding, register_binding_controller)

ICONS = op.dirname(icons.__file__)
CHECKED = op.join(ICONS, "checkbox-checked.svg")
UNCHECKED = op.join(ICONS, "checkbox-unchecked.svg")


# XXX: priority = 10
@register_binding_controller(ui_name='Toggle Field', read_only=True,
                             binding_type=BoolBinding)
class DisplayCheckBox(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(CheckBoxModel)

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        widget.setFixedSize(20, 20)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        svg = CHECKED if value else UNCHECKED
        self.widget.load(svg)
