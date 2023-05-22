# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtWidgets import QBoxLayout

from karabo.common.scenemodel.api import BoxLayoutModel, LabelModel, SceneModel
from karabogui.testing import GuiTestCase

from ...view import SceneView
from ..actions import BoxHSceneAction, BoxVSceneAction


class TestLayoutActions(GuiTestCase):
    def setUp(self):
        super().setUp()
        self._foo_label = LabelModel(x=20, y=20, text="foo")
        self._bar_label = LabelModel(x=10, y=10, text="bar")
        self._baz_label = LabelModel(x=10, y=20, text="baz")
        labels = [self._foo_label, self._bar_label, self._baz_label]
        self._scene_model = SceneModel(children=labels)
        self._scene_view = SceneView(model=self._scene_model)

    def tearDown(self):
        super().tearDown()
        self._scene_view.destroy()

    def test_basics(self):
        self._assert_sceneview_contents(changed=False)

    def test_horz_layout_action(self):
        action = BoxHSceneAction()

        # Not selected, so the values are not changed
        action.perform(self._scene_view)
        self._assert_sceneview_contents(changed=False)
        self._assert_layout_in_scene(model_klass=BoxLayoutModel, valid=False)

        # Select all label models
        self._scene_view.select_all()
        action.perform(self._scene_view)
        self._assert_sceneview_contents(changed=True)
        self._assert_layout_in_scene(model_klass=BoxLayoutModel, valid=True)

        # Check layout model
        layout_model = self._get_layout_model(model_klass=BoxLayoutModel)
        self.assertEqual(layout_model.direction, QBoxLayout.LeftToRight)

        # Check order. The expected is as follows: (x, y)
        # 1. The label bar goes first (10, 10)
        # 2. The label baz goes next (10, 20)
        # 3, The label foo goes last (20, 20)
        labels = [self._bar_label, self._baz_label, self._foo_label]
        self.assertListEqual(layout_model.children, labels)

    def test_vert_layout_action(self):
        action = BoxVSceneAction()

        # Not selected, so the values are not changed
        action.perform(self._scene_view)
        self._assert_sceneview_contents(changed=False)
        self._assert_layout_in_scene(model_klass=BoxLayoutModel, valid=False)

        # Select all label models
        self._scene_view.select_all()
        action.perform(self._scene_view)
        self._assert_sceneview_contents(changed=True)
        self._assert_layout_in_scene(model_klass=BoxLayoutModel, valid=True)

        # Check layout model
        layout_model = self._get_layout_model(model_klass=BoxLayoutModel)
        self.assertEqual(layout_model.direction, QBoxLayout.TopToBottom)

        # Check order. The expected is as follows: (x, y)
        # 1. The label bar goes first (10, 10)
        # 2. The label baz goes next (10, 20)
        # 3, The label foo goes last (20, 20)
        labels = [self._bar_label, self._baz_label, self._foo_label]
        self.assertListEqual(layout_model.children, labels)

    def _assert_sceneview_contents(self, changed=True):
        assertion = self.assertFalse if changed else self.assertTrue

        obj_cache = list(self._scene_view._scene_obj_cache.keys())
        assertion(self._scene_model.children == obj_cache)

    def _assert_layout_in_scene(self, model_klass=None, valid=True):
        assertion = self.assertIsNotNone if valid else self.assertIsNone
        layout_model = self._get_layout_model(model_klass)
        assertion(layout_model)

    def _get_layout_model(self, model_klass=None):
        # Get layout model from scene objects
        for model in self._scene_view._scene_obj_cache.keys():
            if isinstance(model, model_klass):
                # We bail out immediately since the obj cache respects order
                return model
