#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabogui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEvent)
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
        super(ScriptingPanel, self).__init__("Console", allow_closing=True)

        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self.event_map)

    def closeEvent(self, event):
        super(ScriptingPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self.event_map)
            self._stop_ipython()
            self.signalPanelClosed.emit(self.windowTitle())

    def _event_network(self, data):
        connected = data.get('status', False)
        if not connected and self.console:
            self.console.stop()
            self._stop_ipython()

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

    @pyqtSlot(bool)
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
            except IOError:
                self._set_button_state(False)
        else:
            self._stop_ipython()

    @pyqtSlot()
    def _stop_ipython(self):
        if self.console:
            self.mainLayout.removeWidget(self.console)
            self.console.stop()
            self.console.setParent(None)
            self.console = None

    @pyqtSlot()
    def enable_button_cb(self):
        self.acStartIPython.setEnabled(True)

    @pyqtSlot()
    def _set_button_state(self, state):
        action = self.acStartIPython

        if state:
            # always disable the button and wait for client's callback
            action.setEnabled(False)
        action.setChecked(state)
        text = STATUS_TIP[state]
        self.acStartIPython.setToolTip(text)
        self.acStartIPython.setStatusTip(text)
