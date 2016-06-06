#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QLine, QRect


class Line(QLine):
    """ A line which can appear in a scene
    """

    def __init__(self):
        super(Line, self).__init__()


class Rectangle(QRect):
    """ A rectangle which can appear in a scene
    """

    def __init__(self):
        super(Rectangle, self).__init__()
