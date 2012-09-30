#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["CustomViewPanel"]


from customviewwidget import CustomViewWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class CustomViewPanel(QWidget):


    def __init__(self):
        super(CustomViewPanel, self).__init__()

        self.__view = CustomViewWidget(self)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__view)
        
        self.setupActions()


    def setupActions(self):
        text = "Disable moving widgets"
        self.__acTransform = QAction(QIcon(":transform"), text, self)
        self.__acTransform.setToolTip(text)
        self.__acTransform.setStatusTip(text)
        self.__acTransform.setCheckable(True)
        self.__acTransform.setChecked(True)
        self.__acTransform.toggled.connect(self.onTransformActive)


    def setupToolBar(self, toolBar):
        toolBar.addAction(self.__acTransform)


    def onTransformActive(self, isChecked):
        if isChecked:
            text = "Disable moving widgets"
        else:
            text = "Enable moving widgets"
        
        self.__acTransform.setToolTip(text)
        self.__acTransform.setStatusTip(text)
        self.__view.isTransformWidgetActive = isChecked


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

