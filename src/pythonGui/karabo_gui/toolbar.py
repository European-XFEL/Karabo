#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 16, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QToolBar


class ToolBar(QToolBar):
    def __init__(self, title="", parent=None):
        super(ToolBar, self).__init__(title, parent=parent)

        self.setStyleSheet("QToolBar {"
                           "background-color: rgb(180,180,180);"
                           "margin-bottom: 0px;"
                           "}")
        iconSize = QSize(32, 32)
        iconSize *= 0.6
        self.setIconSize(iconSize)

