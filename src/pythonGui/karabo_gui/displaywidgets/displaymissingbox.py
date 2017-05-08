#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 8, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo_gui.util import PlaceholderWidget
from karabo_gui.widget import DisplayWidget


class DisplayMissingBox(DisplayWidget):
    # This is not a valid category ON PURPOSE
    category = DisplayWidget
    # `alias` is missing because we don't want to appear in a context menu
    # Without `alias`, we won't be added to the global widget registry

    def __init__(self, parent):
        super(DisplayMissingBox, self).__init__(None)
        self.widget = PlaceholderWidget('Property Missing', parent)
        self.boxes = []

    @property
    def value(self):
        return None
