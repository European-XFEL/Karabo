#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import unittest

from karabo.common.scenemodel.api import LineModel, RectangleModel
from karabogui.sceneview.selection_model import SceneSelectionModel
from karabogui.sceneview.shapes import LineShape, RectangleShape


class TestSelectionModel(unittest.TestCase):
    def setUp(self):
        self.selection_model = SceneSelectionModel()

    def test_selection_model(self):
        self.assertFalse(self.selection_model.has_selection())
        endpoint_x, endpoint_y = (100, 100)

        line_model = LineModel(x1=0, y1=0, x2=endpoint_x, y2=endpoint_y)
        line_shape = LineShape(model=line_model)
        self.selection_model.select_object(line_shape)
        self.assertTrue(self.selection_model.has_selection())

        rect_model = RectangleModel(x=endpoint_x, y=endpoint_y,
                                    width=100, height=100)
        rect_shape = RectangleShape(model=rect_model)
        self.selection_model.select_object(rect_shape)
        self.assertTrue(self.selection_model.has_selection())

        self.selection_model.clear_selection()
        self.assertFalse(self.selection_model.has_selection())

        self.selection_model.select_object(line_shape)
        self.selection_model.select_object(rect_shape)

        for obj in self.selection_model:
            self.assertIn(obj, (line_shape, rect_shape))

        exp_rect = line_shape.geometry().united(rect_shape.geometry())
        sel_rect = self.selection_model.get_selection_bounds()
        self.assertEqual(exp_rect, sel_rect)

        self.selection_model.deselect_object(line_shape)
        self.assertTrue(self.selection_model.has_selection())

        self.selection_model.deselect_object(rect_shape)
        self.assertFalse(self.selection_model.has_selection())
