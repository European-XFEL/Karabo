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
import pytest
from qtpy.QtWidgets import QBoxLayout

from karabo.common.scenemodel.api import BoxLayoutModel, LabelModel, SceneModel
from karabogui.const import IS_MAC_SYSTEM

from ...view import SceneView
from ..actions import BoxHSceneAction, BoxVSceneAction


@pytest.fixture()
def scene_model(gui_app):
    foo_label = LabelModel(x=20, y=20, text="foo")
    bar_label = LabelModel(x=10, y=10, text="bar")
    baz_label = LabelModel(x=10, y=20, text="baz")
    labels = [foo_label, bar_label, baz_label]
    scene_model = SceneModel(children=labels)
    return scene_model


@pytest.fixture()
def scene_view(gui_app, scene_model):
    scene_view = SceneView(model=scene_model)
    return scene_view


def test_basics(scene_view, scene_model):
    _assert_sceneview_contents(scene_view, scene_model, changed=False)


@pytest.mark.skipif(IS_MAC_SYSTEM, reason="Segfault on MacOS")
def test_horz_layout_action(scene_view, scene_model):
    action = BoxHSceneAction()

    # Not selected, so the values are not changed
    action.perform(scene_view)
    _assert_sceneview_contents(scene_view, scene_model, changed=False)
    _assert_layout_in_scene(scene_view, model_klass=BoxLayoutModel,
                            valid=False)

    # Select all label models
    scene_view.select_all()
    action.perform(scene_view)
    _assert_sceneview_contents(scene_view, scene_model, changed=True)
    _assert_layout_in_scene(scene_view, model_klass=BoxLayoutModel,
                            valid=True)

    # Check layout model
    layout_model = _get_layout_model(scene_view, model_klass=BoxLayoutModel)
    assert layout_model.direction == QBoxLayout.LeftToRight

    # Check order. The expected is as follows: (x, y)
    # 1. The label bar goes first (10, 10)
    # 2. The label baz goes next (10, 20)
    # 3, The label foo goes last (20, 20)
    labels = ["bar", "baz", "foo"]
    texts = [child.text for child in layout_model.children]
    assert labels == texts
    scene_view.destroy()


@pytest.mark.skipif(IS_MAC_SYSTEM, reason="Segfault on MacOS")
def test_vert_layout_action(scene_view, scene_model):
    action = BoxVSceneAction()

    # Not selected, so the values are not changed
    action.perform(scene_view)
    _assert_sceneview_contents(scene_view, scene_model, changed=False)
    _assert_layout_in_scene(scene_view, model_klass=BoxLayoutModel,
                            valid=False)

    # Select all label models
    scene_view.select_all()
    action.perform(scene_view)
    _assert_sceneview_contents(scene_view, scene_model, changed=True)
    _assert_layout_in_scene(scene_view, model_klass=BoxLayoutModel, valid=True)

    # Check layout model
    layout_model = _get_layout_model(scene_view, model_klass=BoxLayoutModel)
    assert layout_model.direction == QBoxLayout.TopToBottom

    # Check order. The expected is as follows: (x, y)
    # 1. The label bar goes first (10, 10)
    # 2. The label baz goes next (10, 20)
    # 3, The label foo goes last (20, 20)
    labels = ["bar", "baz", "foo"]
    texts = [child.text for child in layout_model.children]
    assert labels == texts
    scene_view.destroy()


def _assert_sceneview_contents(view, model, changed=True):
    assertion = not changed
    obj_cache = list(view._scene_obj_cache.keys())
    assert (model.children == obj_cache) == assertion


def _assert_layout_in_scene(view, model_klass=None, valid=True):
    layout_model = _get_layout_model(view, model_klass)
    if valid:
        assert layout_model is not None
    else:
        assert layout_model is None


def _get_layout_model(view, model_klass=None):
    # Get layout model from scene objects
    for model in view._scene_obj_cache.keys():
        if isinstance(model, model_klass):
            # We bail out immediately since the obj cache respects order
            return model
