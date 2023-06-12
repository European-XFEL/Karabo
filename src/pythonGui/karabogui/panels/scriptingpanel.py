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
from functools import partial

from qtpy.QtCore import Slot
from qtpy.QtWidgets import QAction, QVBoxLayout, QWidget

from karabogui.access import AccessRole, access_role_allowed
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.widgets.ipython import IPythonWidget
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget

BANNER_TEXT = "Welcome to the embedded ipython console.\n"
STATUS_TIP = {
    True: "End IPython session",  # session is on
    False: "Start IPython console"  # session is off
}


class ScriptingPanel(BasePanelWidget):
    def __init__(self):
        super().__init__("Console", allow_closing=True)

        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.LoginUserChanged: self._event_access_level,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
        }
        register_for_broadcasts(self.event_map)
        # When we are opened, we check our access!
        self._access_console()

    def closeEvent(self, event):
        super().closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self.event_map)
            self._stop_ipython()
            self.signalPanelClosed.emit(self.windowTitle())

    # Karabo Events
    # -----------------------------------------------------------------------

    def _event_network(self, data):
        connected = data.get('status', False)
        if not connected and self.console:
            self.console.stop()
            self._stop_ipython()

    def _event_access_level(self, data):
        self._access_console()

    # -----------------------------------------------------------------------

    def _access_console(self):
        allowed = access_role_allowed(AccessRole.CONSOLE_EDIT)
        self.acStartIPython.setVisible(allowed)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget()

        self.console = None
        self.mainLayout = QVBoxLayout(widget)
        self.mainLayout.setContentsMargins(5, 5, 5, 5)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        toolbar = ToolBar(parent=self)

        self.acStartIPython = QAction("IP[y]:", toolbar)
        self.acStartIPython.setCheckable(True)
        self._set_button_state(False)
        self.acStartIPython.triggered.connect(self._on_button_pressed)
        toolbar.addAction(self.acStartIPython)

        return [toolbar]

    @Slot(bool)
    def _on_button_pressed(self, isChecked):
        if isChecked:
            # Turn on session
            try:
                stop_cb = partial(self._set_button_state, False)
                call_backs = {'start_ch': self.enable_button_cb,
                              'stop_ch': stop_cb}
                self.console = IPythonWidget(
                    banner=BANNER_TEXT, call_backs=call_backs)
                self.console.exit_requested.connect(self._stop_ipython)
                self.mainLayout.addWidget(self.console)
                self._set_button_state(True)
            except OSError:
                self._set_button_state(False)
        else:
            self._stop_ipython()

    @Slot()
    def _stop_ipython(self):
        if self.console:
            # Stop any operations
            self.console.stop()
            # Remove references from the parent
            self.console.exit_requested.disconnect()
            self.mainLayout.removeWidget(self.console)
            self.console.setParent(None)
            # Finally destroy the widget
            self.console.destroy()
            self.console = None

    @Slot()
    def enable_button_cb(self):
        self.acStartIPython.setEnabled(True)

    @Slot()
    def _set_button_state(self, state):
        action = self.acStartIPython

        if state:
            # always disable the button and wait for client's callback
            action.setEnabled(False)
        action.setChecked(state)
        text = STATUS_TIP[state]
        self.acStartIPython.setToolTip(text)
        self.acStartIPython.setStatusTip(text)
