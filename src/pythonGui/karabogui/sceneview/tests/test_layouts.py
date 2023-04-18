#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import os.path as op

from qtpy.QtWidgets import QBoxLayout

import karabo.common.scenemodel.tests as sm
from karabo.common.scenemodel.api import BoxLayoutModel, LabelModel, LineModel
from karabogui.sceneview.layout.api import BoxLayout
from karabogui.sceneview.shapes import LineShape
from karabogui.sceneview.widget.api import LabelWidget
from karabogui.testing import GuiTestCase

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


class TestLayouts(GuiTestCase):
    def test_horizontal_box_layouts(self):
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
