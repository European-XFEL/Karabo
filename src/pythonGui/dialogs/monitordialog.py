#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 06, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["MonitorDialog"]

import icons

from PyQt4 import uic
#from PyQt4.QtCore import pyqtSlot, QRegExp, Qt, QSize
from PyQt4.QtGui import (QDialog, QDialogButtonBox)

import os.path


class MonitorDialog(QDialog):


    def __init__(self, monitor):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'monitordialog.ui'), self)
        
        if monitor is None:
            title = "Add monitor"
        else:
            title = "Edit monitor"
        
        self.setWindowTitle(title)
        
        self.pbSelectDeviceId.setIcon(icons.deviceInstance)
        self.pbSelectProperty.setIcon(icons.enum)
        
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)


