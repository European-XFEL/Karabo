#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the logging panel of the bottom
   middle of the MainWindow which is un/dockable.
"""

__all__ = ["LoggingPanel"]


import icons
from manager import Manager
from logwidget import LogWidget

from PyQt4.QtGui import QAction, QVBoxLayout, QWidget


class LoggingPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    #
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################


    def __init__(self):
        super(LoggingPanel, self).__init__()

        self.__logWidget = LogWidget(self)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__logWidget)
        
        Manager().signalLogDataAvailable.connect(self.__logWidget.onLogDataAvailable)
        
        self.setupActions()


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
        
    
    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

