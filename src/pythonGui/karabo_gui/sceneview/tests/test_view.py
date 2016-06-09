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
from ..view import SceneView
from ..layouts import GroupLayout

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


class TestSceneView(unittest.TestCase):
    
    '''Test the GUI scene view'''
    def setUp(self):
        '''Create the view'''
        self.app = QApplication(sys.argv)
        self.view = SceneView()

    def test_loading(self):
        '''Test the view loading an SVG file'''
        self.view.load(op.join(DATA_DIR, "all.svg"))
        self.assertEqual(type(self.view.layout), GroupLayout)
