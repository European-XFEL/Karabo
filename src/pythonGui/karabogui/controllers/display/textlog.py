#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 27, 2012
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
from datetime import datetime

from qtpy.QtCore import Qt
from qtpy.QtWidgets import (
    QFrame, QHBoxLayout, QMenu, QPushButton, QTextEdit, QVBoxLayout)
from traits.api import Instance

from karabo.common.scenemodel.api import DisplayTextLogModel
from karabo.native import Timestamp
from karabogui import icons
from karabogui.binding.api import StringBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.generic_scenes import get_property_proxy_model
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName

BUTTON_SIZE = 32


@register_binding_controller(ui_name='Text Log', klassname='DisplayTextLog',
                             binding_type=StringBinding)
class DisplayTextLog(BaseBindingController):
    # The scene model class for this controller
    model = Instance(DisplayTextLogModel, args=())
    # Internal traits
    log_widget = Instance(QTextEdit)
    _timestamp = Instance(Timestamp, args=())

    def create_widget(self, parent):
        widget = QFrame(parent)
        ver_layout = QVBoxLayout(widget)
        ver_layout.setSpacing(0)

        self.log_widget = QTextEdit(widget)

        # no focus for this widget!
        self.log_widget.setFocusPolicy(Qt.NoFocus)
        self.log_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.log_widget.customContextMenuRequested.connect(
            self._show_context_menu)

        ver_layout.addWidget(self.log_widget)
        # start horizontal layout with button and spacer
        hor_layout = QHBoxLayout()

        # strech is required to move the button to the right
        hor_layout.addStretch(1)
        hor_layout.setSpacing(0)

        history_button = QPushButton()
        history_button.setFixedSize(BUTTON_SIZE, BUTTON_SIZE)
        history_button.setFocusPolicy(Qt.NoFocus)
        history_button.setToolTip('Historic Text Log')
        history_button.setIcon(icons.clock)
        history_button.clicked.connect(self.launch_history)
        button = QPushButton()
        button.setFixedSize(BUTTON_SIZE, BUTTON_SIZE)
        button.setFocusPolicy(Qt.NoFocus)
        button.setToolTip('Clear log')
        button.setIcon(icons.editClear)
        button.clicked.connect(self.log_widget.clear)

        hor_layout.addWidget(history_button)
        hor_layout.addWidget(button)

        # spacer item to align button to the right!
        ver_layout.addLayout(hor_layout)

        # nice color background
        objectName = generateObjectName(widget)
        sheet = ('QWidget#{} {{ background-color : rgba{}; }}'
                 ''.format(objectName, ALL_OK_COLOR))
        self.log_widget.setObjectName(objectName)
        self.log_widget.setStyleSheet(sheet)
        return widget

    def value_update(self, proxy):
        # catch both None and empty strings
        value = get_binding_value(proxy, '')
        timestamp = proxy.binding.timestamp
        if value and timestamp != self._timestamp:
            self._write_log(value, timestamp)
            self._timestamp = timestamp

    def _write_log(self, text, timestamp):
        dt = datetime.fromtimestamp(timestamp.toTimestamp())
        stamp = dt.strftime('[%H:%M:%S]')
        item = f'{stamp}: {text}'
        self.log_widget.append(item)

        # update our scroll bar
        bar = self.log_widget.verticalScrollBar()
        bar.setValue(bar.maximum())

    def _show_context_menu(self, pos):
        """Show a context menu"""
        menu = QMenu(self.widget)
        select_action = menu.addAction('Select All')
        select_action.triggered.connect(self.log_widget.selectAll)
        enable_select = len(self.log_widget.toPlainText()) > 0
        select_action.setEnabled(enable_select)

        copy_action = menu.addAction('Copy Selected')
        copy_action.triggered.connect(self.log_widget.copy)
        enable_copy = not self.log_widget.textCursor().selection().isEmpty()
        copy_action.setEnabled(enable_copy)
        menu.exec(self.log_widget.viewport().mapToGlobal(pos))

    def launch_history(self):
        if self.proxy.binding is None:
            return
        model = get_property_proxy_model(self.proxy)
        data = {"model": model}
        broadcast_event(KaraboEvent.ShowUnattachedController, data)
