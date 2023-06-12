#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
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
from qtpy.QtWidgets import QAction, QVBoxLayout, QWidget

from karabogui import icons
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.widgets.log import LogWidget
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget


class LoggingPanel(BasePanelWidget):
    def __init__(self):
        super().__init__("Log", allow_closing=True)
        # Register for broadcast events.
        self.event_map = {
            KaraboEvent.LogMessages: self._event_log_messages,
            KaraboEvent.NetworkConnectStatus: self._event_network,
        }
        register_for_broadcasts(self.event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        self._log_widget = LogWidget(parent=widget)
        mainLayout = QVBoxLayout(widget)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self._log_widget)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        text = "Save log data to file"
        self.__acSaveLog = QAction(icons.save, "&Save log data (.log)", self)
        self.__acSaveLog.setToolTip(text)
        self.__acSaveLog.setStatusTip(text)
        self.__acSaveLog.triggered.connect(self._log_widget.onSaveToFile)

        text = "Clear log"
        self.__acClearLog = QAction(icons.editClear, text, self)
        self.__acClearLog.setToolTip(text)
        self.__acClearLog.setStatusTip(text)
        self.__acClearLog.triggered.connect(self._log_widget.onClearLog)

        toolBar = ToolBar(parent=self)
        toolBar.addAction(self.__acSaveLog)
        toolBar.addAction(self.__acClearLog)
        return [toolBar]

    def closeEvent(self, event):
        """Unregister from broadcast events, tell main window to enable
        the button to add me back.
        """
        super().closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self.event_map)
            self.signalPanelClosed.emit(self.windowTitle())

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_log_messages(self, data):
        messages = data.get('messages', [])
        self._log_widget.onLogDataAvailable(messages)

    def _event_network(self, data):
        if not data['status']:
            # on False status we only clear the logs
            self._log_widget.onClearLog()
