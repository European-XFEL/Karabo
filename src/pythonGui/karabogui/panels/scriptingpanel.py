#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabogui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEventSender)
from karabogui.widgets.ipython import IPythonWidget
from karabogui.widgets.toolbar import ToolBar
from .base import BasePanelWidget

BANNER_TEXT = "Welcome to the embedded ipython console.\n"


class ScriptingPanel(BasePanelWidget):
    def __init__(self):
        super(ScriptingPanel, self).__init__("Console", allow_closing=True)
        register_for_broadcasts(self)

    def closeEvent(self, event):
        super(ScriptingPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self)
            self._stop_ipython()
            self.signalPanelClosed.emit(self.windowTitle())

    def karaboBroadcastEvent(self, event):
        if event.sender is KaraboEventSender.NetworkConnectStatus:
            connected = event.data.get('status', False)
            if not connected and self.console:
                self.console.stop()
                self._stop_ipython()

        return False

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

        text = "Start IPython console"
        self.acStartIPython = QAction("IP[y]:", toolbar)
        self.acStartIPython.setToolTip(text)
        self.acStartIPython.setStatusTip(text)
        self.acStartIPython.triggered.connect(self._on_start_ipython)
        toolbar.addAction(self.acStartIPython)

        return [toolbar]

    @pyqtSlot(bool)
    def _on_start_ipython(self, isChecked):
        if self.console:
            self._stop_ipython()
            # Clicking once when started will stop the console. We cannot
            # directly start a new due to race conditions.
            return
        try:
            self.console = IPythonWidget(banner=BANNER_TEXT)
            self.console.exit_requested.connect(self._stop_ipython)
            self.mainLayout.addWidget(self.console)
        except IOError:
            pass  # connection failed or not online

    @pyqtSlot()
    def _stop_ipython(self):
        if self.console:
            self.mainLayout.removeWidget(self.console)
            self.console.stop()
            self.console.setParent(None)
            self.console = None
