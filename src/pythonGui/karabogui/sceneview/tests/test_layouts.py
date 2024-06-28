#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import os.path as op

from qtpy.QtWidgets import QBoxLayout

import karabo.common.scenemodel.tests as sm
from karabo.common.scenemodel.api import BoxLayoutModel, LabelModel, LineModel
from karabogui.sceneview.layout.api import BoxLayout
from karabogui.sceneview.shapes import LineShape
from karabogui.sceneview.widget.api import LabelWidget

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')


def test_horizontal_box_layouts(gui_app):
    # Horizonal layout
    model = BoxLayoutModel(direction=QBoxLayout.LeftToRight)
    boxLayout = BoxLayout(model, model.direction)
    assert boxLayout.count() == 0
    # Add a child to layout model
    label_model = LabelModel(x=0, y=0, text="foo")
    label_widget = LabelWidget(label_model)
    boxLayout._add_widget(label_widget)
    assert boxLayout.count() == 1
    assert boxLayout.itemAt(0).widget() is label_widget


def test_vertical_box_layout(gui_app):
    # Vertical layout
    model = BoxLayoutModel(direction=QBoxLayout.TopToBottom)
    boxLayout = BoxLayout(model, model.direction)
    assert boxLayout.count() == 0

    model = LineModel(x1=0, y1=0, x2=1, y2=1)
    # Add shape to layout
    lineShape = LineShape(model=model)
    boxLayout._add_shape(lineShape)
    assert boxLayout.count() == 1
    assert boxLayout.itemAt(0).shape is lineShape
