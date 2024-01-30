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
from unittest import mock, skipIf

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
    ALL_PROPERTIES_MAP, GuiTestCase, get_all_props_schema)

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


class TestLoadSVG(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.view = SceneView()

    def _load_file(self, dir, file_name):
        assert self.view.scene_model is None
        assert isinstance(self.view.layout, GroupLayout)

        scene_model = sm.read_scene(op.join(dir, file_name))
        scene_model.children.append(UNKNOWN_XML_MODEL)
        self.view.update_model(scene_model)
        assert isinstance(self.view.scene_model, sm.SceneModel)
        assert self.view.scene_model.width == self.view.width()
        assert self.view.scene_model.height == self.view.height()

    def test_loading_karabo_svg(self):
        self._load_file(DATA_DIR, "all.svg")

    def test_loading_inkscape_svg(self):
        self._load_file(INKSCAPE_DIR, "shapes.svg")


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


class BaseSceneViewTest(GuiTestCase):

    def setUp(self):
        super().setUp()
        # Setup device
        schema = get_all_props_schema()
        binding = build_binding(schema)
        device = DeviceProxy(binding=binding,
                             server_id='Fake',
                             device_id=DEVICE_NAME,
                             status=ProxyStatus.OFFLINE)

        self.proxy_map = {name: PropertyProxy(root_proxy=device, path=name)
                          for name in ALL_PROPS_BINDINGS.values()}

        # Setup scene view
        self.scene_view = SceneView()
        self.scene_model = sm.SceneModel()

    def tearDown(self):
        super().tearDown()
        self.scene_view.destroy()
        self.scene_view = None
        self.scene_model = None

    def assert_model(self, actual, expected):
        pass

    def assert_widget(self, model):
        pass

    def assert_scene_element(self, actual, expected, modified=None):
        self._assert_element(actual, expected, modified=modified)

        # Save and reread the model
        read_model = single_model_round_trip(actual)
        self.set_models_to_scene(read_model)
        self._assert_element(read_model, expected, modified=False)

        # Revert back the original model
        self.set_models_to_scene(actual)

    def _assert_element(self, actual, expected, modified=False):
        # Check model traits
        self.assert_model(actual, expected)

        # Check if model is modified
        if modified is None:
            modified = expected.modified
        assert actual.modified == modified

        # Check widget
        self.assert_widget(actual)

        # Iterate over children
        if self._has_children(actual) and self._has_children(expected):
            iter_children = zip(actual.children, expected.children)
            for act_child, exp_child in iter_children:
                self._assert_element(act_child, exp_child,
                                     modified=modified)

    def _assign_key(self, model):
        controller = get_model_controller(model)
        if controller is None:
            return None

        # Use a compatible binding
        ctrait = controller.class_traits().get("_binding_type")
        binding = self._get_binding(ctrait.default_value()[1])

        model.keys = [f"{DEVICE_NAME}.{ALL_PROPS_BINDINGS[binding]}"]

    def copy_model(self, model):
        copied = model.clone_traits(copy="deep")
        set_initialized_flag(copied, value=model.initialized)
        set_modified_flag(copied, value=model.modified)
        return copied

    def _get_widget(self, model):
        return self.scene_view._scene_obj_cache.get(model)

    def set_models_to_scene(self, *models):
        # Check if there are defs in the the models
        self.scene_model = sm.SceneModel(children=list(models))
        with mock.patch(GET_PROXY_PATH, new=self._get_proxy):
            self.scene_view.update_model(self.scene_model)
            self.scene_view.update()
        return self.scene_model

    def _get_layout_model(self, model, x=10, y=10, modified=False):
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
            self._assign_key(child)
            child.trait_set(x=x0, y=y0)
            # Add child width for the next iteration
            x0 += child.width
        hbox_model.height = height
        hbox_model.width = sum([child.width for child in children_model])
        set_initialized_flag(hbox_model, value=True)
        set_modified_flag(hbox_model, value=modified)
        return hbox_model

    def _get_binding(self, bindings):
        for binding in bindings:
            if binding in ALL_PROPS_BINDINGS:
                return binding

            binding = REIMPLEMENTED_BINDINGS.get(binding)

            # We found a compatible binding
            if binding in ALL_PROPS_BINDINGS:
                return binding

    def _get_proxy(self, _, name):
        """Return a `PropertyProxy` instance for a given device and
        property path. This mocks the get_proxy called by
        ControllerContainer."""
        return self.proxy_map.get(name)

    @staticmethod
    def _has_children(model):
        return "children" in model.trait_names()


class TestSceneView(BaseSceneViewTest):

    def test_basics(self):
        self.scene_view.update_model(self.scene_model)
        scene_geom = self.scene_view.geometry()
        assert scene_geom.width() == self.scene_model.width
        assert scene_geom.height() == self.scene_model.height


class TestLoadSceneModel(BaseSceneViewTest):

    def test_display_widgets(self):
        """Testing the loading of display widgets"""
        self._assert_geometry(sm.CheckBoxModel)
        self._assert_geometry(sm.ColorBoolModel)
        self._assert_geometry(sm.DaemonManagerModel)
        self._assert_geometry(sm.DisplayCommandModel)
        self._assert_geometry(sm.DisplayIconCommandModel)
        self._assert_geometry(sm.DisplayLabelModel)
        self._assert_geometry(sm.DisplayListModel)
        self._assert_geometry(sm.DisplayProgressBarModel)
        self._assert_geometry(sm.DisplayStateColorModel)
        self._assert_geometry(sm.DisplayTextLogModel)
        self._assert_geometry(sm.DisplayTimeModel)
        self._assert_geometry(sm.ErrorBoolModel)
        self._assert_geometry(sm.EvaluatorModel)
        self._assert_geometry(sm.GlobalAlarmModel)
        self._assert_geometry(sm.HexadecimalModel)
        self._assert_geometry(sm.LampModel)
        self._assert_geometry(sm.LineEditModel)
        self._assert_geometry(sm.RunConfiguratorModel)
        self._assert_geometry(sm.SingleBitModel)
        self._assert_geometry(sm.TableElementModel)
        self._assert_geometry(sm.WidgetNodeModel)

    def test_editable_widgets(self):
        """Testing the geometry loading of editable widgets"""
        self._assert_geometry(sm.CheckBoxModel, klass="EditableCheckBox")
        self._assert_geometry(sm.EditableChoiceElementModel)
        self._assert_geometry(sm.EditableComboBoxModel)
        self._assert_geometry(sm.DoubleLineEditModel)
        self._assert_geometry(sm.EditableListElementModel)
        self._assert_geometry(sm.EditableListModel)
        self._assert_geometry(sm.EditableSpinBoxModel)
        self._assert_geometry(sm.FloatSpinBoxModel)
        self._assert_geometry(sm.IntLineEditModel)
        self._assert_geometry(sm.LineEditModel, klass="EditableLineEdit")
        self._assert_geometry(sm.TableElementModel,
                              klass="EditableTableElement")
        self._assert_geometry(sm.TickSliderModel)

    def test_icon_widgets(self):
        self._assert_geometry(sm.DigitIconsModel)
        # Note: IconSet is deprecated and now becomes a ImageRenderer
        # self._assert_geometry(sm.DisplayIconsetModel, data=b"<svg></svg>")
        self._assert_geometry(sm.SelectionIconsModel)
        self._assert_geometry(sm.TextIconsModel)

    def test_image_graph_widgets(self):
        self._assert_geometry(sm.DetectorGraphModel)
        self._assert_geometry(sm.ImageGraphModel)
        self._assert_geometry(sm.WebCamGraphModel)
        self._assert_geometry(sm.VectorRollGraphModel)

    def test_scatter_graph_widgets(self):
        self._assert_geometry(sm.ScatterGraphModel)
        self._assert_geometry(sm.VectorScatterGraphModel)

    def test_text_widgets(self):
        self._assert_geometry(sm.LabelModel)
        self._assert_geometry(sm.StickerModel)

    def test_instance_widget(self):
        self._assert_geometry(sm.InstanceStatusModel,
                              keys=["DEVICE1.deviceId"])

    @skipIf(IS_MAC_SYSTEM, reason="Link widgets are flaky on MacOS")
    def test_link_widgets(self):
        self._assert_geometry(sm.DeviceSceneLinkModel)
        self._assert_geometry(sm.SceneLinkModel)
        self._assert_geometry(sm.WebLinkModel)

    def test_tool_widgets(self):
        self._assert_geometry(sm.ImageRendererModel,
                              image=sm.create_base64image("svg", b"karabo"))

    def test_trend_graph_widgets(self):
        self._assert_geometry(sm.AlarmGraphModel)
        self._assert_geometry(sm.StateGraphModel)
        self._assert_geometry(sm.TrendGraphModel)

    def test_vector_graph_widgets(self):
        self._assert_geometry(sm.MultiCurveGraphModel)
        self._assert_geometry(sm.NDArrayGraphModel)
        self._assert_geometry(sm.VectorGraphModel)
        self._assert_geometry(sm.VectorHistGraphModel)
        self._assert_geometry(sm.VectorBarGraphModel)
        self._assert_geometry(sm.VectorFillGraphModel)
        self._assert_geometry(sm.VectorXYGraphModel)

    # ----------------------------------------------------------------------
    # Assertions

    def _assert_geometry(self, model_klass, **traits):
        assertions = [self._assert_single_model, self._assert_valid_layout]
        for assertion in assertions:
            assertion(model_klass(**traits))
        # Dead layouts mean that the children widgets do not follow the desired
        # geometry. We let Qt handle such cases, which is as such on the
        # previous implementations. There's no need to check this.
        # self._assert_dead_layout(model_klass, **traits)

    def _assert_single_model(self, model):
        model.trait_set(x=10, y=10, width=100, height=30)
        self._assign_key(model)
        set_initialized_flag(model, value=True)
        expected = self.copy_model(model)

        self.set_models_to_scene(model)
        self.assert_scene_element(model, expected)

        # Translate the widget
        self._apply_actual_changes(model)
        self._apply_expected_changes(expected)
        self.assert_scene_element(model, expected, modified=True)

    def _assert_valid_layout(self, model):
        actual = self._get_layout_model(model)
        expected = self.copy_model(actual)

        self.set_models_to_scene(actual)
        self.assert_scene_element(actual, expected, modified=False)

        # Translate the widget
        self._apply_actual_changes(actual)
        self._apply_expected_changes(expected)
        self.assert_scene_element(actual, expected, modified=True)

    def _assert_dead_layout(self, model):
        """Dead layouts mean that the children widgets do not follow the
        desired geometry, with the dimensions saved smaller and the
        position always at (0, 0). We let Qt handle such cases, which is
        as such on the previous implementations. There's no need to check
        this.
        """
        # Initialize expected models
        expected = self._get_layout_model(model, modified=True)

        # Initialize actual models. We set the position of children widgets
        # to (0, 0)
        actual = self.copy_model(expected)
        for child in actual.children:
            child.trait_setq(x=0, y=0)

        # -- Test the model
        self.set_models_to_scene(actual)
        self.assert_scene_element(actual, expected)

        # Translate the widget
        self._apply_actual_changes(actual)
        self._apply_expected_changes(expected)
        self.assert_scene_element(actual, expected, modified=True)

    # ----------------------------------------------------------------------
    # Reimplemented methods

    def assert_model(self, actual, expected):
        # Check geom traits
        geom_traits = ["x", "y", "width", "height"]
        actual_values = actual.trait_get(geom_traits)
        expected_values = expected.trait_get(geom_traits)
        assert actual_values == expected_values

    def assert_widget(self, model):
        widget = self._get_widget(model)
        scene_geom = widget.geometry()
        model_geom = model.trait_get("x", "y", "width", "height")
        assert scene_geom.x() == model_geom.get("x", 0)
        assert scene_geom.y() == model_geom.get("y", 0)
        assert scene_geom.width() == model_geom.get("width")
        assert scene_geom.height() == model_geom.get("height")

    # ----------------------------------------------------------------------
    # Helper methods

    def _apply_actual_changes(self, actual):
        # Set the offset of the actual model
        widget = self._get_widget(actual)
        widget.translate(QPoint(*OFFSET))

    def _apply_expected_changes(self, expected):
        # Set the offset of the expected model
        def set_offset(model):
            model.x += OFFSET[0]
            model.y += OFFSET[1]

        set_offset(expected)
        if self._has_children(expected):
            for child in expected.children:
                set_offset(child)


SCENE_VIEW_PATH = "karabogui.sceneview.view.SceneView"
MAP_FROM_GLOBAL_PATH = SCENE_VIEW_PATH + ".mapFromGlobal"
VISIBLE_REGION_PATH = SCENE_VIEW_PATH + ".visibleRegion"
REPLACE_DIALOG_PATH = "karabogui.sceneview.tools.clipboard.ReplaceDialog"


class TestClipboardActions(BaseSceneViewTest):

    def test_widgets(self):
        """These are standalone widgets that are not bound to any devices."""
        # Test single model selection
        label_model = self._get_label_model("foo")
        self._assert_copy_paste(label_model)

        # Test single layout selection
        layout_model = self._get_layout_model(label_model)
        self._assert_clipboard_action(layout_model)

        # Test multiple models selection
        another_label = self._get_label_model("bar", x=10, y=10)
        self._assert_clipboard_action(label_model, another_label)

        # Test multiple layouts selection
        another_layout = self._get_layout_model(another_label, x=10, y=10)
        self._assert_clipboard_action(layout_model, another_layout)

    def test_shapes(self):
        # Test single line shape
        line_model = sm.LineModel(x1=10, y1=20, x2=100, y2=100)
        self._assert_clipboard_action(line_model)

        # Test single arrow shape
        arrow_model = sm.ArrowModel(x1=30, y1=40, x2=300, y2=400)
        self._assert_clipboard_action(arrow_model)

        # Test multiple shapes
        self._assert_clipboard_action(line_model, arrow_model)

    # ----------------------------------------------------------------------
    # Assertions

    def _assert_clipboard_action(self, *models):
        self._assert_cut_paste(*models)
        self._assert_cut_pastereplace(*models)
        self._assert_copy_paste(*models)
        self._assert_copy_pastereplace(*models)

    def _assert_cut_paste(self, *models, pos=(30, 30)):
        with self._setup_and_test_selection(*models, pos=pos):
            SceneCutAction().perform(self.scene_view)
            self._paste_models(pos=pos)

        self._assert_cut_models(*models)

    def _assert_cut_pastereplace(self, *models, pos=(30, 30)):
        with self._setup_and_test_selection(*models, pos=pos):
            SceneCutAction().perform(self.scene_view)
            self._pastereplace_models(*models, pos=pos)
        self._assert_cut_models(*models)

    def _assert_cut_models(self, *models):
        copied_models = self._get_selected_models()
        assert len(self.scene_model.children) == len(models)
        # Check if the copied models are in the scene
        for copied in copied_models:
            assert copied in self.scene_model.children
        # Check if the models are deleted in the scene
        for model in models:
            assert model not in self.scene_model.children

    def _assert_copy_paste(self, *models, pos=(30, 30)):
        with self._setup_and_test_selection(*models, pos=pos):
            SceneCopyAction().perform(self.scene_view)
            self._paste_models(pos=pos)

        self._assert_copied_models(*models)

    def _assert_copy_pastereplace(self, *models, pos=(30, 30)):
        with self._setup_and_test_selection(*models, pos=pos):
            SceneCopyAction().perform(self.scene_view)
            self._pastereplace_models(*models, pos=pos)

        self._assert_copied_models(*models)

    def _assert_copied_models(self, *models):
        copied_models = self._get_selected_models()
        assert len(self.scene_model.children) == len(models) * 2
        for model in copied_models + list(models):
            assert model in self.scene_model.children

    # ----------------------------------------------------------------------
    # Reimplemented methods

    def assert_model(self, actual, expected, offset=(0, 0)):
        trait_names = expected.copyable_trait_names()

        # Recurse over the children models
        if "children" in trait_names:
            trait_names.remove("children")
            iter_children = zip(actual.children, expected.children)
            for actual_child, expected_child in iter_children:
                self.assert_model(actual_child, expected_child, offset=offset)

        # Check children models that are not linked as children (e.g., marker)
        if "marker" in trait_names:
            trait_names.remove("marker")
            self.assert_model(actual.marker, expected.marker)

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
        copied = self.copy_model(expected)
        add_offset(copied, x=offset[0], y=offset[1])

        # Check if the rest of the traits are equivalent
        assert type(actual), type(expected)
        assert actual.trait_get(trait_names) == \
               copied.trait_get(trait_names)

    # ----------------------------------------------------------------------
    # Helper methods

    def _paste_models(self, pos=(0, 0)):
        with self._setup_paste_action(pos):
            ScenePasteAction().perform(self.scene_view)

    def _pastereplace_models(self, *models, device_id="NewDevice", pos=(0, 0)):
        mapped_device_ids = {DEVICE_NAME: device_id}

        # patch replace dialog
        exec_patch = mock.patch(REPLACE_DIALOG_PATH + ".exec",
                                return_value=QDialog.Accepted)
        mapped_patch = mock.patch(REPLACE_DIALOG_PATH + ".mappedDevices",
                                  return_value=mapped_device_ids)
        with self._setup_paste_action(pos), exec_patch, mapped_patch:
            action = ScenePasteReplaceAction()
            action.perform(self.scene_view)
            # Correct the models for checking later
            for m in models:
                action._set_widget_model_keys(m, mapped_device_ids)

    @staticmethod
    def _get_label_model(text, x=0, y=0):
        FONT_SPEC = 'Source Sans Pro,48,-1,5,50,0,0,0,0,0'
        traits = {'x': x, 'y': y, 'height': 100, 'width': 100,
                  'text': text, 'font': FONT_SPEC,
                  'foreground': '#000000', 'background': '#ffffff',
                  'frame_width': 0}
        return sm.LabelModel(**traits)

    @contextmanager
    def _setup_and_test_selection(self, *models, pos=(0, 0)):
        try:
            # Reset scene view
            self.set_models_to_scene()
            assert len(self.scene_model.children) == 0

            # Add models to scene
            self.set_models_to_scene(*models)
            assert len(self.scene_model.children) == len(models)
            # Copy-paste
            self.scene_view.select_models(*models)
            yield
        finally:
            # Checked copied model has same traits as read model
            copied_models = self._get_selected_models()
            x0, y0 = calc_relative_pos(models)
            offset = (pos[0] - x0, pos[1] - y0)
            for copied_model, read_model in zip(copied_models, models):
                self.assert_model(copied_model, read_model, offset=offset)

    @contextmanager
    def _setup_paste_action(self, pos=(0, 0)):
        try:
            scene_view = self.scene_view
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

    def _get_selected_models(self):
        return [o.model for o in self.scene_view.selection_model]
