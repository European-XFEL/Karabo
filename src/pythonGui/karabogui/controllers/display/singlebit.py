#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 10, 2017
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
from numpy import log2
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QInputDialog, QLabel, QSizePolicy
from traits.api import Instance, Str, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import SingleBitModel
from karabogui.binding.api import IntBinding, get_binding_value, get_min_max
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.indicators import STATE_COLORS
from karabogui.util import generateObjectName


@register_binding_controller(ui_name='Single Bit', klassname='SingleBit',
                             binding_type=IntBinding)
class SingleBit(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(SingleBitModel, args=())
    # Internal traits
    _internal_widget = Instance(QLabel)
    _style_sheet = Str

    def create_widget(self, parent):
        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setFocusPolicy(Qt.NoFocus)
        self._internal_widget.setMinimumSize(14, 8)
        self._internal_widget.setSizePolicy(QSizePolicy.Expanding,
                                            QSizePolicy.Expanding)

        objectName = generateObjectName(self)
        self._internal_widget.setObjectName(objectName)
        self._style_sheet = (f"QLabel#{objectName}" +
                             " {{ background-color: rgb{}; "
                             "border: 2px solid black;"
                             "border-radius: 6px;}}")
        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)

        def _flip_invert():
            self.model.invert = not self.model.invert

        logicAction = QAction("Invert color logic", widget)
        logicAction.triggered.connect(_flip_invert)
        changeAction = QAction("Change Bit...", widget)
        changeAction.triggered.connect(self._on_change_bit)
        widget.addAction(logicAction)
        widget.addAction(changeAction)
        return widget

    def binding_update(self, proxy):
        self.widget.update_unit_label(proxy)

    def set_read_only(self, ro):
        self._internal_widget.setEnabled(not ro)

    def value_update(self, proxy):
        value = int(get_binding_value(proxy, 0))
        value = (value >> self.model.bit) & 1 != 0
        value = not value if self.model.invert else value

        color = (STATE_COLORS[State.ACTIVE] if value
                 else STATE_COLORS[State.PASSIVE])
        style = self._style_sheet.format(color)
        self._internal_widget.setStyleSheet(style)

    @on_trait_change("model.invert,model.bit", post_init=True)
    def _model_update(self):
        self.value_update(self.proxy)

    def _on_change_bit(self):
        binding = self.proxy.binding
        if binding is None:
            return

        dt = binding.displayType
        if dt.startswith("bin|"):
            s, ok = QInputDialog.getItem(self.widget, "Select Bit",
                                         "Select Bit:", dt[4:].split(","))
            if ok:
                bit = int(s.split(":")[0])
        else:
            _, high = get_min_max(binding)
            max_value = int(log2(high) + 1)
            bit, ok = QInputDialog.getInt(
                self.widget, "Bit Number", "Enter number of bit:",
                self.model.bit, 0, max_value)

        if ok:
            self.model.bit = bit
