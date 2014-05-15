#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__author__="kerstin weger"

# export PYTHONPATH= <pathToExfelSuite>/lib/debug

import sip
sip.setapi("QString", 2)
sip.setapi("QVariant", 2)
sip.setapi("QUrl", 2)

import sys
import icons

from mainwindow import MainWindow
from network import Network

from PyQt4.QtGui import QApplication

import numpy

if __name__ == '__main__':
    numpy.set_printoptions(suppress=True, threshold=10)
    app = QApplication(sys.argv)
    icons.init()

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
    
    from displaywidgets import *
    from editablewidgets import *
    from vacuumwidgets import *
        
    window = MainWindow()
    # Make connections between Network and MainWindow
    window.signalServerConnection.connect(Network().onServerConnection)
    window.signalQuitApplication.connect(Network().onQuitApplication)
    Network().signalServerConnectionChanged.connect(window.onServerConnectionChanged)
    Network().signalUserChanged.connect(window.onUpdateAccessLevel)
    
    sys.exit(app.exec_())
