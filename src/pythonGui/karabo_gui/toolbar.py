#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 16, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the a standard QToolBar which
   should be used in this project.
"""

__all__ = ["ToolBar"]


from PyQt4.QtCore import QSize
from PyQt4.QtGui import QToolBar


class ToolBar(QToolBar):

    
    def __init__(self, title=""):
        super(ToolBar, self).__init__(title)

        self.setStyleSheet("QToolBar {"
                           "background-color: rgb(180,180,180);"
                           "margin-bottom: 0px;"
                           "}")
        iconSize = QSize(32,32)
        iconSize *= 0.6
        self.setIconSize(iconSize)

