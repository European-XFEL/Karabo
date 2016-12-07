#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
import karabo_gui.icons as icons
from karabo_gui.logwidget import LogWidget


class LoggingPanel(Dockable, QWidget):
    def __init__(self):
        super(LoggingPanel, self).__init__()

        self.__logWidget = LogWidget(self)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.__logWidget)

        self.setupActions()

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.LogMessages:
                messages = event.data.get('messages', [])
                self.__logWidget.onLogDataAvailable(messages)
            return False
        return super(LoggingPanel, self).eventFilter(obj, event)

    def setupActions(self):
        text = "Save log data to file"
        self.__acSaveLog = QAction(icons.save, "&Save log data (.log)", self)
        self.__acSaveLog.setToolTip(text)
        self.__acSaveLog.setStatusTip(text)
        self.__acSaveLog.triggered.connect(self.__logWidget.onSaveToFile)

        text = "Clear log"
        self.__acClearLog = QAction(icons.editClear, text, self)
        self.__acClearLog.setToolTip(text)
        self.__acClearLog.setStatusTip(text)
        self.__acClearLog.triggered.connect(self.__logWidget.onClearLog)

    def setupToolBars(self, toolBar, parent):
        toolBar.addAction(self.__acSaveLog)
        toolBar.addAction(self.__acClearLog)
