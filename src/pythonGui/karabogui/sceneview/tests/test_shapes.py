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

import karabo.common.scenemodel.tests as sm
from karabo.common.scenemodel.api import (
    LineModel, PathModel, RectangleModel, SceneModel)
from karabogui.testing import GuiTestCase

from ..api import SceneView

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')


class TestShapes(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.view = SceneView()
        self.view._set_scene_model(SceneModel())

    def test_line_shape(self):
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = LineModel(x1=0, y1=0, x2=1, y2=1)
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)

    def test_rect_shape(self):
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = RectangleModel(x=0, y=0, width=100, height=100)
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)

    def test_path_shape(self):
        self.assertEqual(len(self.view._scene_obj_cache), 0)
        model = PathModel()
        model.svg_data = ("M 111.42857,389.50504 285.71429,546.6479 "
                          "428.57143,398.07647")
        self.view.add_models(model)
        self.assertEqual(len(self.view._scene_obj_cache), 1)
        self.assertIn(model, self.view._scene_obj_cache)
