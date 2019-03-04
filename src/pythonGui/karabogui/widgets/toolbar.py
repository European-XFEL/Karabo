#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 16, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QSize
from PyQt4.QtGui import QSizePolicy, QToolBar, QWidget


class ToolBar(QToolBar):
    def __init__(self, title="", parent=None):
        super(ToolBar, self).__init__(title, parent=parent)

        iconSize = QSize(32, 32)
        iconSize *= 0.6
        self.setIconSize(iconSize)

    def add_expander(self):
        """Add an empty widget to the toolbar which absorbs horizontal space.
        """
        expander = QWidget()
        expander.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.addWidget(expander)
