#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op

import karabo_gui.scenemodel.tests as sm
from karabo_gui.scenemodel.api import (LineModel, RectangleModel, PathModel,
                                       SceneModel)
from karabo_gui.sceneview.view import SceneView
from karabo_gui.testing import GuiTestCase

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')


class TestShapes(GuiTestCase):
    """ Test the GUI shapes"""

    def setUp(self):
        """ Create the view"""
        super(TestShapes, self).setUp()
        self.view = SceneView()
        self.view._set_scene_model(SceneModel())

    def test_line_shape(self):
        """ Test the line shape"""
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = LineModel(x1=0, y1=0, x2=1, y2=1)
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)

    def test_rect_shape(self):
        """ Test the rectangle shape"""
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = RectangleModel(x=0, y=0, width=100, height=100)
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)

    def test_path_shape(self):
        """ Test the path shape"""
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = PathModel()
        model.svg_data = ("M 111.42857,389.50504 285.71429,546.6479 "
                          "428.57143,398.07647")
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)
