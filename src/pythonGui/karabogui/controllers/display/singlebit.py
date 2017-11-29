#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from numpy import log2
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QInputDialog, QLabel
from traits.api import Instance, Str, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import SingleBitModel
from karabogui.binding.api import (
    IntBinding, get_min_max, KARABO_SCHEMA_DISPLAY_TYPE)
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.indicators import STATE_COLORS
from karabogui.util import generateObjectName


@register_binding_controller(ui_name='Single Bit', klassname='SingleBit',
                             binding_type=IntBinding)
class SingleBit(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(SingleBitModel)
    # Internal traits
    _internal_widget = Instance(QLabel)
    _style_sheet = Str

    def create_widget(self, parent):
        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setFocusPolicy(Qt.NoFocus)
        self._internal_widget.setFixedSize(24, 24)

        objectName = generateObjectName(self)
        self._internal_widget.setObjectName(objectName)
        self._style_sheet = ("QLabel#{}".format(objectName) +
                             " {{ background-color: rgba{}; "
                             "border: 2px solid black;"
                             "border-radius: 12px; }} ")

        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)

        @pyqtSlot()
        def _flip_invert():
            self.model.invert = not self.model.invert

        logicAction = QAction('Invert color logic', widget)
        logicAction.triggered.connect(_flip_invert)
        changeAction = QAction('Change Bit...', widget)
        changeAction.triggered.connect(self._on_change_bit)
        widget.addAction(logicAction)
        widget.addAction(changeAction)
        return widget

    def set_read_only(self, ro):
        self._internal_widget.setEnabled(not ro)

    @on_trait_change('proxy:value,model.invert,model.bit', post_init=True)
    def _value_update(self):
        if self.proxy.binding is None:
            return

        value = (self.proxy.value >> self.model.bit) & 1 != 0
        value = not value if self.model.invert else value

        color = (STATE_COLORS[State.ACTIVE] if value
                 else STATE_COLORS[State.PASSIVE])
        style = self._style_sheet.format(color)
        self._internal_widget.setStyleSheet(style)
        self.widget.update_label(self.proxy)

    @pyqtSlot()
    def _on_change_bit(self):
        binding = self.proxy.binding
        if binding is None:
            return

        dt = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE, '')
        if dt.startswith('bin|'):
            s, ok = QInputDialog.getItem(self.widget, 'Select Bit',
                                         'Select Bit:', dt[4:].split(','))
            if ok:
                bit = int(s.split(':')[0])
        else:
            _, high = get_min_max(binding)
            bit, ok = QInputDialog.getInt(
                self.widget, "Bit Number", "Enter number of bit:",
                self.model.bit, 0, log2(high) + 1)

        if ok:
            self.model.bit = bit
