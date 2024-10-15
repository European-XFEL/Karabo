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
from contextlib import contextmanager
from unittest import mock

import pytest
from qtpy.QtCore import QPoint
from qtpy.QtGui import QRegion
from qtpy.QtWidgets import QBoxLayout, QDialog

import karabo.common.scenemodel.api as sm
import karabo.common.scenemodel.tests as sm_tests
from karabo.common.api import set_initialized_flag, set_modified_flag
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabogui.binding.api import (
    DeviceProxy, ImageBinding, Int8Binding, IntBinding, NDArrayBinding,
    NodeBinding, PipelineOutputBinding, PropertyProxy, ProxyStatus,
    SignedIntBinding, TableBinding, Uint8Binding, UnsignedIntBinding,
    VectorBinding, VectorHashBinding, VectorNoneBinding, VectorNumberBinding,
    VectorUint8Binding, WidgetNodeBinding, build_binding)
from karabogui.const import IS_MAC_SYSTEM
from karabogui.controllers.registry import get_model_controller
from karabogui.testing import (
    ALL_PROPERTIES_MAP, assert_no_throw, get_all_props_schema)

from ..api import SceneView
from ..layout.api import GroupLayout
from ..tools.clipboard import (
    SceneCopyAction, SceneCutAction, ScenePasteAction, ScenePasteReplaceAction)
from ..utils import add_offset, calc_relative_pos

DATA_DIR = op.join(op.abspath(op.dirname(sm_tests.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')
UNKNOWN_XML_MODEL = sm.UnknownXMLDataModel(
    tag='circle',
    attributes={'cx': '100', 'cy': '100', 'r': '100'}
)


def load_file(view, dir_, file_name):
    assert view.scene_model is None
    assert isinstance(view.layout, GroupLayout)

    scene_model = sm.read_scene(op.join(dir_, file_name))
    scene_model.children.append(UNKNOWN_XML_MODEL)
    view.update_model(scene_model)
    assert isinstance(view.scene_model, sm.SceneModel)
    assert view.scene_model.width == view.width()
    assert view.scene_model.height == view.height()


def test_loading_karabo_svg(gui_app):
    view = SceneView()
    load_file(view, DATA_DIR, "all.svg")


def test_loading_inkscape_svg(gui_app):
    view = SceneView()
    load_file(view, INKSCAPE_DIR, "shapes.svg")


OFFSET = (10, 20)
GET_PROXY_PATH = "karabogui.sceneview.widget.container.get_proxy"
DEVICE_NAME = "Device"
ALL_PROPS_BINDINGS = {binding: name
                      for name, binding in ALL_PROPERTIES_MAP.items()}
REIMPLEMENTED_BINDINGS = {
    ImageBinding: NodeBinding,
    IntBinding: Int8Binding,
    NDArrayBinding: NodeBinding,
    PipelineOutputBinding: NodeBinding,
    SignedIntBinding: Int8Binding,
    TableBinding: VectorHashBinding,
    UnsignedIntBinding: Uint8Binding,
    VectorNumberBinding: VectorUint8Binding,
    VectorNoneBinding: VectorBinding,
    WidgetNodeBinding: NodeBinding}
SCENE_VIEW_PATH = "karabogui.sceneview.view.SceneView"
MAP_FROM_GLOBAL_PATH = SCENE_VIEW_PATH + ".mapFromGlobal"
VISIBLE_REGION_PATH = SCENE_VIEW_PATH + ".visibleRegion"
REPLACE_DIALOG_PATH = "karabogui.sceneview.tools.clipboard.ReplaceDialog"


# -----------------------------------------------------------------------------
# Fixtures:

@pytest.fixture
def setup_scene_view(gui_app):
    # Setup device
    schema = get_all_props_schema()
    binding = build_binding(schema)
    device = DeviceProxy(binding=binding,
                         server_id='Fake',
                         device_id=DEVICE_NAME,
                         status=ProxyStatus.OFFLINE)

    proxy_map = {name: PropertyProxy(root_proxy=device, path=name)
                 for name in ALL_PROPS_BINDINGS.values()}

    # Setup scene view
    scene_view = SceneView()
    scene_model = sm.SceneModel()
    yield proxy_map, scene_view, scene_model
    scene_view.destroy()


@pytest.fixture
def setup_load_scene_model(setup_scene_view):
    proxy_map, scene_view, _ = setup_scene_view
    yield proxy_map, scene_view
    # additional teardown
    with assert_no_throw():
        scene_view._update_widget_states()


# -----------------------------------------------------------------------------
# Assertions:

def assert_scene_element(scene_view, proxy_map, actual, expected,
                         modified=None):
    assert_element(scene_view, actual, expected, modified=modified)

    # Save and reread the model
    read_model = single_model_round_trip(actual)
    set_models_to_scene(scene_view, proxy_map, read_model)
    assert_element(scene_view, read_model, expected, modified=False)

    # Revert back the original model
    set_models_to_scene(scene_view, proxy_map, actual)


def assert_element(scene_view, actual, expected, modified=False):
    # Check geom traits
    geom_traits = ["x", "y", "width", "height"]
    actual_values = actual.trait_get(geom_traits)
    expected_values = expected.trait_get(geom_traits)
    assert actual_values == expected_values

    # Check if model is modified
    if modified is None:
        modified = expected.modified
    assert actual.modified == modified

    # Check widget
    widget = scene_view._scene_obj_cache.get(actual)
    scene_geom = widget.geometry()
    model_geom = actual.trait_get("x", "y", "width", "height")
    assert scene_geom.x() == model_geom.get("x", 0)
    assert scene_geom.y() == model_geom.get("y", 0)
    assert scene_geom.width() == model_geom.get("width")
    assert scene_geom.height() == model_geom.get("height")

    # Iterate over children
    actual_traits = actual.trait_names()
    expected_traits = expected.trait_names()
    if "children" in actual_traits and "children" in expected_traits:
        iter_children = zip(actual.children, expected.children)
        for act_child, exp_child in iter_children:
            assert_element(scene_view, act_child, exp_child, modified=modified)


def assert_geometry(scene_view, proxy_map, model_klass, **traits):

    def assert_single_model(model):
        model.trait_set(x=10, y=10, width=100, height=30)
        assign_key(model)
        set_initialized_flag(model, value=True)
        expected = copy_model(model)

        set_models_to_scene(scene_view, proxy_map, model)
        assert_scene_element(scene_view, proxy_map, model, expected)

        # Translate the widget
        apply_changes(model, expected)
        assert_scene_element(scene_view, proxy_map, model, expected,
                             modified=True)

    def assert_valid_layout(model):
        actual = get_layout_model(model)
        expected = copy_model(actual)

        set_models_to_scene(scene_view, proxy_map, actual)
        assert_scene_element(scene_view, proxy_map, actual, expected,
                             modified=False)

        # Translate the widget
        apply_changes(actual, expected)
        assert_scene_element(scene_view, proxy_map, actual, expected,
                             modified=True)

    def apply_changes(actual, expected):
        # Set the offset of the actual model
        widget = scene_view._scene_obj_cache.get(actual)
        widget.translate(QPoint(*OFFSET))

        expected.x += OFFSET[0]
        expected.y += OFFSET[1]
        if "children" in expected.trait_names():
            for child in expected.children:
                child.x += OFFSET[0]
                child.y += OFFSET[1]

    assertions = [assert_single_model, assert_valid_layout]
    for assertion in assertions:
        assertion(model_klass(**traits))
    # Dead layouts mean that the children widgets do not follow the desired
    # geometry. We let Qt handle such cases, which is as such on the
    # previous implementations. There's no need to check this.


def assert_clipboard_action(proxy_map, scene_view, *models):
    pos = (30, 30)
    # cut and paste
    with setup_and_test_selection(proxy_map, scene_view, *models,
                                  pos=pos) as scene_model:
        SceneCutAction().perform(scene_view)
        with setup_paste_action(scene_view, pos):
            ScenePasteAction().perform(scene_view)
    assert_cut_models(scene_view, scene_model, *models)

    # cut and paste with replacing
    with setup_and_test_selection(proxy_map, scene_view, *models,
                                  pos=pos) as scene_model:
        SceneCutAction().perform(scene_view)
        pastereplace_models(scene_view, *models, pos=pos)
    assert_cut_models(scene_view, scene_model, *models)

    # copy and paste
    with setup_and_test_selection(proxy_map, scene_view, *models,
                                  pos=pos) as scene_model:
        SceneCopyAction().perform(scene_view)
        with setup_paste_action(scene_view, pos):
            ScenePasteAction().perform(scene_view)
    assert_copied_models(scene_view, scene_model, *models)

    # copy and paste with replacing
    with setup_and_test_selection(proxy_map, scene_view, *models,
                                  pos=pos) as scene_model:
        SceneCopyAction().perform(scene_view)
        pastereplace_models(scene_view, *models, pos=pos)
    assert_copied_models(scene_view, scene_model, *models)


def assert_cut_models(scene_view, scene_model, *models):
    copied_models = [o.model for o in scene_view.selection_model]
    assert len(scene_model.children) == len(models)
    # Check if the copied models are in the scene
    for copied in copied_models:
        assert copied in scene_model.children
    # Check if the models are deleted in the scene
    for model in models:
        assert model not in scene_model.children


def assert_copied_models(scene_view, scene_model, *models):
    copied_models = [o.model for o in scene_view.selection_model]
    assert len(scene_model.children) == len(models) * 2
    for model in copied_models + list(models):
        assert model in scene_model.children


def assert_model(actual, expected, offset=(0, 0)):
    trait_names = expected.copyable_trait_names()

    # Recurse over the children models
    if "children" in trait_names:
        trait_names.remove("children")
        iter_children = zip(actual.children, expected.children)
        for actual_child, expected_child in iter_children:
            assert_model(actual_child, expected_child, offset=offset)

    # Check children models that are not linked as children (e.g., marker)
    if "marker" in trait_names:
        trait_names.remove("marker")
        assert_model(actual.marker, expected.marker)

    # Check if IDs are not the same since we generate a new ID for the
    # pasted models
    if "id" in trait_names:
        trait_names.remove("id")
        is_valid = actual.id != expected.id
        # If actual.id is blank, the valid is for the expected.id
        # to be blank also.
        if not actual.id:
            is_valid = not is_valid
        assert is_valid

    # Correct expected position
    copied = copy_model(expected)
    add_offset(copied, x=offset[0], y=offset[1])

    # Check if the rest of the traits are equivalent
    assert type(actual), type(expected)
    assert actual.trait_get(trait_names) == copied.trait_get(trait_names)


# -----------------------------------------------------------------------------
# Helper functions:

def assign_key(model):
    controller = get_model_controller(model)
    if controller is None:
        return None
    # Use a compatible binding
    ctrait = controller.class_traits().get("_binding_type")
    binding = get_binding(ctrait.default_value()[1])
    model.keys = [f"{DEVICE_NAME}.{ALL_PROPS_BINDINGS[binding]}"]


def copy_model(model):
    copied = model.clone_traits(copy="deep")
    set_initialized_flag(copied, value=model.initialized)
    set_modified_flag(copied, value=model.modified)
    return copied


def set_models_to_scene(scene_view, proxy_map, *models):

    def get_proxy(_, name):
        """Return a `PropertyProxy` instance for a given device and
        property path. This mocks the get_proxy called by
        ControllerContainer."""
        return proxy_map.get(name)

    # Check if there are defs in the models
    scene_model = sm.SceneModel(children=list(models))
    with mock.patch(GET_PROXY_PATH, new=get_proxy):
        scene_view.update_model(scene_model)
        scene_view.update()
    return scene_model


def get_layout_model(model, x=10, y=10, modified=False):
    # Initialize children models
    height = 30
    label_model = sm.LabelModel(width=100, height=height, text="bar")
    display_model = sm.DisplayLabelModel(width=200, height=height)
    model.trait_set(width=300, height=height)
    children_model = [label_model, display_model, model]

    # Initialize layout model
    hbox_model = sm.BoxLayoutModel(x=x, y=y, children=children_model,
                                   direction=QBoxLayout.LeftToRight)

    # Correct the position from the initial position and width
    x0, y0 = (x, y)
    for child in children_model:
        assign_key(child)
        child.trait_set(x=x0, y=y0)
        # Add child width for the next iteration
        x0 += child.width
    hbox_model.height = height
    hbox_model.width = sum([child.width for child in children_model])
    set_initialized_flag(hbox_model, value=True)
    set_modified_flag(hbox_model, value=modified)
    return hbox_model


def get_binding(bindings):
    for binding in bindings:
        if binding in ALL_PROPS_BINDINGS:
            return binding

        binding = REIMPLEMENTED_BINDINGS.get(binding)

        # We found a compatible binding
        if binding in ALL_PROPS_BINDINGS:
            return binding


def pastereplace_models(scene_view, *models, device_id="NewDevice",
                        pos=(0, 0)):
    mapped_device_ids = {DEVICE_NAME: device_id}

    # patch replace dialog
    exec_patch = mock.patch(REPLACE_DIALOG_PATH + ".exec",
                            return_value=QDialog.Accepted)
    mapped_patch = mock.patch(REPLACE_DIALOG_PATH + ".mappedDevices",
                              return_value=mapped_device_ids)
    with setup_paste_action(scene_view, pos), exec_patch, mapped_patch:
        action = ScenePasteReplaceAction()
        action.perform(scene_view)
        # Correct the models for checking later
        for m in models:
            action._set_widget_model_keys(m, mapped_device_ids)


def get_label_model(text, x=0, y=0):
    FONT_SPEC = 'Source Sans Pro,48,-1,5,50,0,0,0,0,0'
    traits = {'x': x, 'y': y, 'height': 100, 'width': 100,
              'text': text, 'font': FONT_SPEC,
              'foreground': '#000000', 'background': '#ffffff',
              'frame_width': 0}
    return sm.LabelModel(**traits)


@contextmanager
def setup_and_test_selection(proxy_map, scene_view, *models, pos=(0, 0)):
    try:
        # Reset scene view
        scene_model = set_models_to_scene(scene_view, proxy_map)
        assert len(scene_model.children) == 0

        # Add models to scene
        scene_model = set_models_to_scene(scene_view, proxy_map, *models)
        assert len(scene_model.children) == len(models)
        # Copy-paste
        scene_view.select_models(*models)
        yield scene_model
    finally:
        # Checked copied model has same traits as read model
        copied_models = [o.model for o in scene_view.selection_model]
        x0, y0 = calc_relative_pos(models)
        offset = (pos[0] - x0, pos[1] - y0)
        for copied_model, read_model in zip(copied_models, models):
            assert_model(copied_model, read_model, offset=offset)


@contextmanager
def setup_paste_action(scene_view, pos=(0, 0)):
    try:
        # patch cursor method
        map_from_global_patch = mock.patch(MAP_FROM_GLOBAL_PATH,
                                           return_value=QPoint(*pos))
        # patch visible region
        region = QRegion(0, 0, scene_view.width(), scene_view.height())
        visible_region_patch = mock.patch(VISIBLE_REGION_PATH,
                                          return_value=region)
        with map_from_global_patch, visible_region_patch:
            yield
    finally:
        pass


# -----------------------------------------------------------------------------
# Scene view tests:

def test_scene_view_basics(setup_scene_view):
    _, scene_view, scene_model = setup_scene_view
    scene_view.update_model(scene_model)
    scene_geom = scene_view.geometry()
    assert scene_geom.width() == scene_model.width
    assert scene_geom.height() == scene_model.height


# -----------------------------------------------------------------------------
# Scene load model tests:

def test_display_widgets(setup_load_scene_model):
    """Testing the loading of display widgets"""
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.CheckBoxModel)
    assert_geometry(scene_view, proxy_map, sm.ColorBoolModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayCommandModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayIconCommandModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayLabelModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayListModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayProgressBarModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayStateColorModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayTextLogModel)
    assert_geometry(scene_view, proxy_map, sm.DisplayTimeModel)
    assert_geometry(scene_view, proxy_map, sm.ErrorBoolModel)
    assert_geometry(scene_view, proxy_map, sm.EvaluatorModel)
    assert_geometry(scene_view, proxy_map, sm.GlobalAlarmModel)
    assert_geometry(scene_view, proxy_map, sm.HexadecimalModel)
    assert_geometry(scene_view, proxy_map, sm.LampModel)
    assert_geometry(scene_view, proxy_map, sm.LineEditModel)
    assert_geometry(scene_view, proxy_map, sm.SingleBitModel)
    assert_geometry(scene_view, proxy_map, sm.TableElementModel)
    assert_geometry(scene_view, proxy_map, sm.WidgetNodeModel)


def test_editable_widgets(setup_load_scene_model):
    """Testing the geometry loading of editable widgets"""
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.CheckBoxModel,
                    klass="EditableCheckBox")
    assert_geometry(scene_view, proxy_map, sm.EditableChoiceElementModel)
    assert_geometry(scene_view, proxy_map, sm.EditableComboBoxModel)
    assert_geometry(scene_view, proxy_map, sm.DoubleLineEditModel)
    assert_geometry(scene_view, proxy_map, sm.EditableListElementModel)
    assert_geometry(scene_view, proxy_map, sm.EditableListModel)
    assert_geometry(scene_view, proxy_map, sm.EditableSpinBoxModel)
    assert_geometry(scene_view, proxy_map, sm.FloatSpinBoxModel)
    assert_geometry(scene_view, proxy_map, sm.IntLineEditModel)
    assert_geometry(scene_view, proxy_map, sm.LineEditModel,
                    klass="EditableLineEdit")
    assert_geometry(scene_view, proxy_map, sm.TableElementModel,
                    klass="EditableTableElement")
    assert_geometry(scene_view, proxy_map, sm.TickSliderModel)


def test_icon_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.DigitIconsModel)
    # Note: IconSet is deprecated and now becomes a ImageRenderer
    # assert_geometry(scene_view, proxy_map, sm.DisplayIconsetModel,
    #                 data=b"<svg></svg>")
    assert_geometry(scene_view, proxy_map, sm.SelectionIconsModel)
    assert_geometry(scene_view, proxy_map, sm.TextIconsModel)


def test_image_graph_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.DetectorGraphModel)
    assert_geometry(scene_view, proxy_map, sm.ImageGraphModel)
    assert_geometry(scene_view, proxy_map, sm.WebCamGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorRollGraphModel)


def test_scatter_graph_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.ScatterGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorScatterGraphModel)


def test_text_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.LabelModel)
    assert_geometry(scene_view, proxy_map, sm.StickerModel)


def test_instance_widget(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.InstanceStatusModel,
                    keys=["DEVICE1.deviceId"])


@pytest.mark.skipif(IS_MAC_SYSTEM, reason="Link widgets are flaky on MacOS")
def test_link_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.DeviceSceneLinkModel)
    assert_geometry(scene_view, proxy_map, sm.SceneLinkModel)
    assert_geometry(scene_view, proxy_map, sm.WebLinkModel)


def test_tool_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.ImageRendererModel,
                    image=sm.create_base64image("svg", b"karabo"))


def test_trend_graph_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.AlarmGraphModel)
    assert_geometry(scene_view, proxy_map, sm.StateGraphModel)
    assert_geometry(scene_view, proxy_map, sm.TrendGraphModel)


def test_vector_graph_widgets(setup_load_scene_model):
    proxy_map, scene_view = setup_load_scene_model
    assert_geometry(scene_view, proxy_map, sm.MultiCurveGraphModel)
    assert_geometry(scene_view, proxy_map, sm.NDArrayGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorHistGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorBarGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorFillGraphModel)
    assert_geometry(scene_view, proxy_map, sm.VectorXYGraphModel)

# -----------------------------------------------------------------------------
# Clipboard actions test:


def test_widgets(setup_scene_view):
    """These are standalone widgets that are not bound to any devices."""
    proxy_map, scene_view, _ = setup_scene_view
    # Test single model selection
    label_model = get_label_model("foo")
    pos = (30, 30)
    with setup_and_test_selection(proxy_map, scene_view, label_model,
                                  pos=pos) as scene_model:
        SceneCopyAction().perform(scene_view)
        with setup_paste_action(scene_view, pos):
            ScenePasteAction().perform(scene_view)
    assert_copied_models(scene_view, scene_model, label_model)

    # Test single layout selection
    layout_model = get_layout_model(label_model)
    assert_clipboard_action(proxy_map, scene_view, layout_model)

    # Test multiple models selection
    another_label = get_label_model("bar", x=10, y=10)
    assert_clipboard_action(proxy_map, scene_view, label_model, another_label)

    # Test multiple layouts selection
    another_layout = get_layout_model(another_label, x=10, y=10)
    assert_clipboard_action(proxy_map, scene_view, layout_model,
                            another_layout)


def test_shapes(setup_scene_view):
    proxy_map, scene_view, _ = setup_scene_view
    # Test single line shape
    line_model = sm.LineModel(x1=10, y1=20, x2=100, y2=100)
    assert_clipboard_action(proxy_map, scene_view, line_model)

    # Test single arrow shape
    arrow_model = sm.ArrowPolygonModel(x1=30, y1=40, x2=300, y2=400)
    assert_clipboard_action(proxy_map, scene_view, arrow_model)

    # Test multiple shapes
    assert_clipboard_action(proxy_map, scene_view, line_model, arrow_model)
