#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__author__="kerstin weger"
# export PYTHONPATH= <pathToExfelSuite>/lib/debug

import util # assure sip api is set first
from traceback import print_exception, format_exception

from async import EventLoop, Macro, DeviceClient
from mainwindow import MainWindow
from network import Network
from manager import Manager

import karabo

from PyQt4.QtGui import QMessageBox

import numpy

import asyncio

from displaywidgets import *
from editablewidgets import *
from vacuumwidgets import *

import icons

def init(argv):
    numpy.set_printoptions(suppress=True, threshold=10)
    loop = EventLoop(argv)
    asyncio.set_event_loop(loop)
    karabo.Macro = Macro
    karabo.DeviceClient = DeviceClient
    karabo.__all__.extend(["Macro", "DeviceClient"])
    icons.init()

    loop.app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")
    
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

    global window
    window = MainWindow()
    window.signalQuitApplication.connect(Network().onQuitApplication)
    window.signalQuitApplication.connect(Manager().closeDatabaseConnection)
    Manager().signalUpdateScenes.connect(window.onUpdateScenes)
    Network().signalServerConnectionChanged.connect(
        window.onServerConnectionChanged)
    Network().signalUserChanged.connect(window.onUpdateAccessLevel)
    window.show()
    return loop


def excepthook(type, value, traceback):
    print_exception(type, value, traceback)
    mb = QMessageBox(getattr(value, "icon", QMessageBox.Critical),
                     getattr(value, "title", type.__name__),
                     "{}\n{}\n".format(getattr(value, "message",
                                  "{}: {}".format(type.__name__, value)),
                                       " " * 300 + "\n"))
    text = "".join(format_exception(type, value, traceback))
    mb.setDetailedText(text)
    mb.exec_()
    try:
        Network().onError(text)
    except Exception:
        print("could not sent exception to network")
