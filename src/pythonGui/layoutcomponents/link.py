#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a component for the middle
   panel which links two items.
"""

__all__ = ["Link"]

from layoutcomponents.linkbase import LinkBase

from PyQt4.QtCore import *


class Link(LinkBase):

    def __init__(self, fromItem, toItem):
        super(Link, self).__init__(fromItem, toItem)


    def __del__(self):
        print "Link.__del__"
        LinkBase.__del__(self)


    def trackItems(self):
        self.setLine(QLineF(self.fromItem.pos(), self.toItem.pos()))

