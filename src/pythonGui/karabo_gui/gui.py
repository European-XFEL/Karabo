#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traceback import print_exception, format_exception

from karabo_gui.singletons.api import get_network, get_manager

# Global MainWindow singleton
window = None


def init(app):
    """ Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    from karabo_gui.mainwindow import MainWindow
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

    # Initialize the Manager singleton
    get_manager()
    network = get_network()

    global window
    window = MainWindow()
    window.signalQuitApplication.connect(network.onQuitApplication)
    network.signalServerConnectionChanged.connect(
        window.onServerConnectionChanged)
    network.signalUserChanged.connect(window.onUpdateAccessLevel)
    window.show()


def excepthook(type, value, traceback):
    from PyQt4.QtGui import QMessageBox

    print_exception(type, value, traceback)
    mb = QMessageBox(getattr(value, "icon", QMessageBox.Critical),
                     getattr(value, "title", type.__name__),
                     "{}\n{}\n".format(getattr(value, "message",
                                  "{}: {}".format(type.__name__, value)),
                                       " " * 300 + "\n"))
    text = "".join(format_exception(type, value, traceback))
    mb.setDetailedText(text)
    # mb.exec_()
    try:
        network = get_network()
        network.onError(text)
    except Exception:
        print("could not send exception to network")
