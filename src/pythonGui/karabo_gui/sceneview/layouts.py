#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from .utils import save_painter_state


class BaseLayout(object):

    def __init__(self):
        super(BaseLayout, self).__init__()
        self.shapes = []
        self.widgets = []

    def add_shape(self, shape):
        self.shapes.append(shape)

    def add_widget(self, widget):
        self.widgets.append(widget)

    def draw(self, painter):
        for shape in self.shapes:
            with save_painter_state(painter):
                shape.draw(painter)
