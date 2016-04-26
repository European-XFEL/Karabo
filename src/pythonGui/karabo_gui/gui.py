#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__author__="kerstin weger"
# export PYTHONPATH= <pathToExfelSuite>/lib/debug

from traceback import print_exception, format_exception


def init(app):
    """ Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    from karabo_gui.mainwindow import MainWindow
    from karabo_gui.network import Network
    from karabo_gui.topology import Manager
    import karabo # XXX: I think this could be for side effects?
    import karabo_gui.gui_registry_loader # XXX: Only imported for side-effects
    import karabo_gui.icons as icons
    import numpy

    numpy.set_printoptions(suppress=True, threshold=10)
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

    global window
    window = MainWindow()
    window.signalQuitApplication.connect(Network().onQuitApplication)
    Manager().signalUpdateScenes.connect(window.onUpdateScenes)
    Network().signalServerConnectionChanged.connect(
        window.onServerConnectionChanged)
    Network().signalUserChanged.connect(window.onUpdateAccessLevel)
    window.show()


def excepthook(type, value, traceback):
    from PyQt4.QtGui import QMessageBox

    from karabo_gui.network import Network

    print_exception(type, value, traceback)
    mb = QMessageBox(getattr(value, "icon", QMessageBox.Critical),
                     getattr(value, "title", type.__name__),
                     "{}\n{}\n".format(getattr(value, "message",
                                  "{}: {}".format(type.__name__, value)),
                                       " " * 300 + "\n"))
    text = "".join(format_exception(type, value, traceback))
    mb.setDetailedText(text)
    #mb.exec_()
    try:
        Network().onError(text)
    except Exception:
        print("could not send exception to network")
