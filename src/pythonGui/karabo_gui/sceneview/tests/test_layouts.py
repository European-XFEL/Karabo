#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op

from PyQt4.QtGui import QBoxLayout

import karabo_gui.scenemodel.tests as sm
from karabo_gui.scenemodel.api import BoxLayoutModel, LabelModel, LineModel
from karabo_gui.sceneview.layout.api import BoxLayout
from karabo_gui.sceneview.shapes import LineShape
from karabo_gui.sceneview.widget.api import LabelWidget
from karabo_gui.testing import GuiTestCase

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


class TestLayouts(GuiTestCase):
    """ Test the GUI layouts"""

    def test_horizontal_box_layouts(self):
        """ Test the horizontal box layout"""
        # Horizonal layout
        model = BoxLayoutModel(direction=QBoxLayout.LeftToRight)
        boxLayout = BoxLayout(model, model.direction)
        self.assertEqual(boxLayout.count(), 0)
        # Add a child to layout model
        label_model = LabelModel(x=0, y=0, text="foo")
        label_widget = LabelWidget(label_model)
        boxLayout._add_widget(label_widget)
        self.assertEqual(boxLayout.count(), 1)
        self.assertIs(boxLayout.itemAt(0).widget(), label_widget)

    def test_vertical_box_layout(self):
        """ Test the vertical box layout"""
        # Vertical layout
        model = BoxLayoutModel(direction=QBoxLayout.TopToBottom)
        boxLayout = BoxLayout(model, model.direction)
        self.assertEqual(boxLayout.count(), 0)

        model = LineModel(x1=0, y1=0, x2=1, y2=1)
        # Add shape to layout
        lineShape = LineShape(model=model)
        boxLayout._add_shape(lineShape)
        self.assertEqual(boxLayout.count(), 1)
        self.assertIs(boxLayout.itemAt(0).shape, lineShape)
