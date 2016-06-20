#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QRect

import unittest

from karabo_gui.scenemodel.shapes import LineModel, RectangleModel
from karabo_gui.sceneview.selection_model import SceneSelectionModel
from karabo_gui.sceneview.shapes import LineShape, RectangleShape


class TestSelectionModel(unittest.TestCase):
    """ Test the GUI selection model"""

    def setUp(self):
        """ Create the selection model"""
        self.selection_model = SceneSelectionModel()

    def test_selection_model(self):
        """ Test the selection model"""
        self.assertFalse(self.selection_model.has_selection())

        line_model = LineModel(x1=0, y1=0, x2=100, y2=100)
        line_shape = LineShape(line_model)
        self.selection_model.select_object(line_shape)
        self.assertTrue(self.selection_model.has_selection())

        rect_model = RectangleModel(x=100, y=100, width=100, height=100)
        rect_shape = RectangleShape(rect_model)
        self.selection_model.select_object(rect_shape)
        self.assertTrue(self.selection_model.has_selection())

        self.selection_model.clear_selection()
        self.assertFalse(self.selection_model.has_selection())

        self.selection_model.select_object(line_shape)
        self.selection_model.select_object(rect_shape)

        for obj in self.selection_model:
            self.assertIn(obj, (line_shape, rect_shape))

        exp_rect = QRect(0, 0, 199, 199)
        sel_rect = self.selection_model.get_selection_bounds()
        self.assertEqual(exp_rect, sel_rect)

        self.selection_model.deselect_object(line_shape)
        self.assertTrue(self.selection_model.has_selection())

        self.selection_model.deselect_object(rect_shape)
        self.assertFalse(self.selection_model.has_selection())
