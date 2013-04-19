#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 2, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a thread."""

__all__ = ["WorkThread"]

import os
from PyQt4.QtCore import *

class WorkThread(QThread):
    
    def __init__(self, cmd):
        QThread.__init__(self)

        self.cmd = cmd

    def __del__(self):
        self.wait()

    def run(self):
        print "work thread run..."
        os.system(self.cmd)
