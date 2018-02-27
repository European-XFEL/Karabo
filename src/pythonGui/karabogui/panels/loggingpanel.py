#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabogui import icons
from karabogui.events import (
    register_for_broadcasts, KaraboEventSender, unregister_from_broadcasts
)
from karabogui.widgets.log import LogWidget
from karabogui.widgets.toolbar import ToolBar
from .base import BasePanelWidget


class LoggingPanel(BasePanelWidget):
    def __init__(self):
        super(LoggingPanel, self).__init__("Log", allow_closing=True)
        # Register for broadcast events.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        self._log_widget = LogWidget(widget)
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

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
        if event.sender is KaraboEventSender.LogMessages:
            messages = event.data.get('messages', [])
            self._log_widget.onLogDataAvailable(messages)

        elif event.sender is KaraboEventSender.NetworkConnectStatus:
            data = event.data
            # on False status we only clear the logs
            if not data['status']:
                self._log_widget.onClearLog()

        return False

    def closeEvent(self, event):
        """Unregister from broadcast events, tell main window to enable
        the button to add me back.
        """
        super(LoggingPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self)
            self.signalPanelClosed.emit(self.windowTitle())
