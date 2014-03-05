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

import sys
import icons

from mainwindow import MainWindow

from PyQt4.QtGui import *

if __name__ == '__main__':
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
    sys.exit(app.exec_())
