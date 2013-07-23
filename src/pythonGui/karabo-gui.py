#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__author__="kerstin weger"

# export PYTHONPATH= <pathToExfelSuite>/lib/debug

import globals
import sys

from displaywidget import DisplayWidget
from editablewidget import EditableWidget
from mainwindow import MainWindow
from vacuumwidget import VacuumWidget

from PyQt4.QtGui import *

def scanWidgetPlugins():
    #TODO: start in thread
    
    #while True:
    # Register available widgets
    # DisplayWidgets
    DisplayWidget.registerAvailableWidgets()
    # EditableWidgets
    EditableWidget.registerAvailableWidgets()
    # VacuumWidgets
    VacuumWidget.registerAvailableWidgets()
    

if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")
    
#    app.setStyleSheet(""
#        "exfel--gui--DockWindow exfel--gui--DivWidget {"
#        "border-style: solid;"
#        "border: 1px solid gray;"
#        "border-radius: 6px;"
#        "}"
#        "exfel--gui--DivWidget QToolBar {"
#        "background-color: rgb(110,110,110);"
#        "margin-bottom: 0px;"
#        "}")
    
    scanWidgetPlugins()
    globals.init()
        
    window = MainWindow()
    sys.exit(app.exec_())
