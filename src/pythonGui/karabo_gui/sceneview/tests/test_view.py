#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op
import sys
import unittest

from PyQt4.QtGui import QApplication

import karabo_gui.scenemodel.tests as sm
from karabo_gui.scenemodel.const import SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT
from karabo_gui.scenemodel.model import SceneModel
from karabo_gui.sceneview.view import SceneView
from karabo_gui.sceneview.layout.api import GroupLayout

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')


class TestSceneView(unittest.TestCase):

    '''Test the GUI scene view'''
    def setUp(self):
        '''Create the view'''
        self.app = QApplication(sys.argv)
        self.view = SceneView()

    def test_loading_karabo_svg(self):
        '''Test the view loading a karabo SVG file'''
        self.assertIsNone(self.view.scene_model)
        self.assertIsInstance(self.view.layout, GroupLayout)

        file_name = "all.svg"
        self.view.load(op.join(DATA_DIR, file_name))
        self.assertEqual(self.view.title, op.basename(file_name))
        self.assertIsInstance(self.view.scene_model, SceneModel)
        self.assertEqual(max(self.view.scene_model.width, SCENE_MIN_WIDTH),
                         self.view.width())
        self.assertEqual(max(self.view.scene_model.height, SCENE_MIN_HEIGHT),
                         self.view.height())

    def test_loading_inkscape_svg(self):
        '''Test the view loading a inkscape SVG file'''
        self.assertIsNone(self.view.scene_model, None)
        file_name = "shapes.svg"
        self.view.load(op.join(INKSCAPE_DIR, "shapes.svg"))
        self.assertEqual(self.view.title, op.basename(file_name))
        self.assertIsInstance(self.view.scene_model, SceneModel)
        self.assertEqual(max(self.view.scene_model.width, SCENE_MIN_WIDTH),
                         self.view.width())
        self.assertEqual(max(self.view.scene_model.height, SCENE_MIN_HEIGHT),
                         self.view.height())
