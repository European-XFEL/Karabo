#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 24, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QObject


class Mediator(QObject):
    """ An empty class which only needs to be a QObject so that it can route
    QEvents.

    See ``karabo_gui.mediator`` for more details.
    """
