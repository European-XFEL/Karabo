#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from karabo.common.scenemodel.api import (
    read_scene, SceneModel, UnknownXMLDataModel)
import karabo.common.scenemodel.tests as sm
from karabogui.testing import GuiTestCase
from ..layout.api import GroupLayout
from ..api import SceneView

DATA_DIR = op.join(op.abspath(op.dirname(sm.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')
UNKNOWN_XML_MODEL = UnknownXMLDataModel(
    tag='circle',
    attributes={'cx': '100', 'cy': '100', 'r': '100'}
)


class TestSceneView(GuiTestCase):
    def setUp(self):
        super(TestSceneView, self).setUp()
        self.view = SceneView()

    def _load_file(self, dir, file_name):
        self.assertIsNone(self.view.scene_model)
        self.assertIsInstance(self.view.layout, GroupLayout)

        scene_model = read_scene(op.join(dir, file_name))
        scene_model.children.append(UNKNOWN_XML_MODEL)
        self.view.update_model(scene_model)
        self.assertIsInstance(self.view.scene_model, SceneModel)
        self.assertEqual(self.view.scene_model.width, self.view.width())
        self.assertEqual(self.view.scene_model.height, self.view.height())

    def test_loading_karabo_svg(self):
        self._load_file(DATA_DIR, "all.svg")

    def test_loading_inkscape_svg(self):
        self._load_file(INKSCAPE_DIR, "shapes.svg")
