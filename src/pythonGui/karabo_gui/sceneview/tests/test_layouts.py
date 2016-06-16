#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op
import sys
import unittest

from PyQt4.QtGui import QApplication, QBoxLayout, QWidget

import karabo_gui.scenemodel.tests as sm
from karabo_gui.scenemodel.shapes import LineModel
from karabo_gui.sceneview.layouts import BoxLayout, GroupLayout, GridLayout 
from karabo_gui.sceneview.shapes import LineShape

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


class TestLayouts(unittest.TestCase):
    
    '''Test the GUI scene view'''
    def setUp(self):
        '''Create the view'''
        self.app = QApplication(sys.argv)

    def test_defaults(self):
        '''Test the layouts'''
        #groupLayout = GroupLayout()
        
        boxLayout = BoxLayout(QBoxLayout.LeftToRight)
        self.assertEqual(boxLayout.count(), 0)
        # Add widget to layout
        w = QWidget()
        boxLayout.add_widget(w, None)
        self.assertEqual(boxLayout.count(), 1)
        self.assertIs(boxLayout.itemAt(0).widget(), w)
    
        self.assertEqual(len(boxLayout.shapes), 0)
        model = LineModel(x1 = 0, y1 = 0, x2 = 1, y2 = 1)
        # Add shape to layout
        lineShape = LineShape(model)
        boxLayout.add_shape(lineShape)
        self.assertEqual(len(boxLayout.shapes), 1)
        self.assertIs(boxLayout.shapes[0], lineShape)
        
        #gridLayout = GridLayout()
        
