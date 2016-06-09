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
from ..layouts import GroupLayout
from ..shapes import BaseShape
from ..view import SceneView

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')


class TestShapes(unittest.TestCase):
    
    '''Test the GUI scene view'''
    def setUp(self):
        '''Create the view'''
        self.app = QApplication(sys.argv)

    def test_defaults(self):
        '''Test the view in its default state'''

