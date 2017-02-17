#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
import karabo_gui.icons as icons
from karabo_gui.logwidget import LogWidget
from karabo_gui.toolbar import ToolBar
from .base import BasePanelWidget


class LoggingPanel(BasePanelWidget):
    def __init__(self):
        super(LoggingPanel, self).__init__("Log")

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        self.__logWidget = LogWidget(widget)
        mainLayout = QVBoxLayout(widget)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.__logWidget)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
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

        toolBar = ToolBar(parent=self)
        toolBar.addAction(self.__acSaveLog)
        toolBar.addAction(self.__acClearLog)
        return [toolBar]

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.LogMessages:
                messages = event.data.get('messages', [])
                self.__logWidget.onLogDataAvailable(messages)
            return False
        return super(LoggingPanel, self).eventFilter(obj, event)
