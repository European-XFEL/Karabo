#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op
import sys
import unittest

from PyQt4.QtGui import QApplication, QBoxLayout

import karabo_gui.scenemodel.tests as sm
from karabo_gui.scenemodel.layouts import BoxLayoutModel
from karabo_gui.scenemodel.shapes import LineModel
from karabo_gui.scenemodel.simple_widgets import LabelModel
from karabo_gui.sceneview.layout.api import BoxLayout
from karabo_gui.sceneview.shapes import LineShape
from karabo_gui.sceneview.simple_widgets import LabelWidget

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


class TestLayouts(unittest.TestCase):

    '''Test the GUI scene view'''
    def setUp(self):
        '''Create the view'''
        self.app = QApplication(sys.argv)

    def test_horizontal_box_layouts(self):
        '''Test the horizontal box layout'''
        # Horizonal layout
        model = BoxLayoutModel()
        model.direction = QBoxLayout.LeftToRight
        boxLayout = BoxLayout(model, model.direction)
        self.assertEqual(boxLayout.count(), 0)
        # Add a child to layout model
        label_model = LabelModel()
        label_model.x = 0
        label_model.y = 0
        label_model.text = "foo"
        label_widget = LabelWidget(label_model)
        boxLayout._add_widget(label_widget)
        self.assertEqual(boxLayout.count(), 1)
        self.assertIs(boxLayout.itemAt(0).widget(), label_widget)

    def test_vertical_box_layout(self):
        '''Test the vertical box layout'''
        # Vertical layout
        model = BoxLayoutModel()
        model.direction = QBoxLayout.TopToBottom
        boxLayout = BoxLayout(model, model.direction)
        self.assertEqual(boxLayout.count(), 0)

        model = LineModel(x1=0, y1=0, x2=1, y2=1)
        # Add shape to layout
        lineShape = LineShape(model)
        boxLayout._add_shape(lineShape)
        self.assertEqual(boxLayout.count(), 1)
        self.assertIs(boxLayout.itemAt(0).shape, lineShape)
