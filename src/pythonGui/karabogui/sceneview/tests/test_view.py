#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op
from platform import system
from unittest import mock, skipIf

from PyQt5.QtCore import QPoint
from PyQt5.QtWidgets import QBoxLayout

from karabo.common.api import (
    ProxyStatus, set_initialized_flag, set_modified_flag)
import karabo.common.scenemodel.api as sm
import karabo.common.scenemodel.tests as sm_tests
from karabo.common.scenemodel.tests.utils import single_model_round_trip

from karabogui.binding.api import (
    build_binding, DeviceProxy, ImageBinding, IntBinding, Int8Binding,
    NDArrayBinding, NodeBinding, PipelineOutputBinding, PropertyProxy,
    SignedIntBinding, StringBinding, TableBinding, Uint8Binding,
    UnsignedIntBinding, VectorBinding, VectorHashBinding, VectorNoneBinding,
    VectorNumberBinding, VectorUint8Binding, WidgetNodeBinding)
from karabogui.binding.tests.schema import (
    ALL_PROPERTIES_MAP, get_all_props_schema)
from karabogui.controllers.registry import get_model_controller
from karabogui.testing import GuiTestCase
from ..layout.api import GroupLayout
from ..api import SceneView

DATA_DIR = op.join(op.abspath(op.dirname(sm_tests.__file__)), 'data')
INKSCAPE_DIR = op.join(DATA_DIR, 'inkscape')
UNKNOWN_XML_MODEL = sm.UnknownXMLDataModel(
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

        scene_model = sm.read_scene(op.join(dir, file_name))
        scene_model.children.append(UNKNOWN_XML_MODEL)
        self.view.update_model(scene_model)
        self.assertIsInstance(self.view.scene_model, sm.SceneModel)
        self.assertEqual(self.view.scene_model.width, self.view.width())
        self.assertEqual(self.view.scene_model.height, self.view.height())

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


@skipIf(system() == "Darwin", reason="Segfault in MacOS")
class TestLoadSceneModel(GuiTestCase):

    def setUp(self):
        super(TestLoadSceneModel, self).setUp()
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
        super(TestLoadSceneModel, self).setUp()
        self.scene_view.destroy()
        self.scene_view = None
        self.scene_model = None

    def test_basic(self):
        self.scene_view.update_model(self.scene_model)
        scene_geom = self.scene_view.geometry()
        self.assertEqual(scene_geom.width(), self.scene_model.width)
        self.assertEqual(scene_geom.height(), self.scene_model.height)

    def test_text_widgets(self):
        """These are standalone widgets that are not bound to any devices."""
        self._assert_geometry(sm.LabelModel)
        self._assert_geometry(sm.SceneLinkModel)
        self._assert_geometry(sm.StickerModel)
        self._assert_geometry(sm.WebLinkModel)

    def test_display_widgets(self):
        self._assert_geometry(sm.AnalogModel)
        self._assert_geometry(sm.CheckBoxModel)
        self._assert_geometry(sm.ChoiceElementModel)
        self._assert_geometry(sm.ColorBoolModel)
        self._assert_geometry(sm.DaemonManagerModel)
        self._assert_geometry(sm.DeviceSceneLinkModel)
        self._assert_geometry(sm.DirectoryModel)
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
        self._assert_geometry(sm.FileInModel)
        self._assert_geometry(sm.FileOutModel)
        self._assert_geometry(sm.GlobalAlarmModel)
        self._assert_geometry(sm.HexadecimalModel)
        self._assert_geometry(sm.LampModel)
        self._assert_geometry(sm.LineEditModel)
        self._assert_geometry(sm.PopUpModel)
        self._assert_geometry(sm.RunConfiguratorModel)
        self._assert_geometry(sm.SingleBitModel)
        self._assert_geometry(sm.TableElementModel)
        self._assert_geometry(sm.WidgetNodeModel)

    def test_editable_widgets(self):
        self._assert_geometry(sm.CheckBoxModel, klass="EditableCheckBox")
        self._assert_geometry(sm.ChoiceElementModel,
                              klass="EditableChoiceElement")
        self._assert_geometry(sm.ComboBoxModel, klass="EditableComboBox")
        self._assert_geometry(sm.DirectoryModel, klass="EditableDirectory")
        self._assert_geometry(sm.DoubleLineEditModel)
        self._assert_geometry(sm.EditableListElementModel)
        self._assert_geometry(sm.EditableListModel)
        self._assert_geometry(sm.EditableSpinBoxModel)
        self._assert_geometry(sm.FileInModel, klass="EditableFileIn")
        self._assert_geometry(sm.FileOutModel, klass="EditableFileOut")
        self._assert_geometry(sm.FloatSpinBoxModel)
        self._assert_geometry(sm.IntLineEditModel)
        self._assert_geometry(sm.LineEditModel, klass="EditableLineEdit")
        self._assert_geometry(sm.TableElementModel,
                              klass="EditableTableElement")
        self._assert_geometry(sm.TickSliderModel)

    def test_graph_widgets(self):
        self._assert_geometry(sm.AlarmGraphModel)
        self._assert_geometry(sm.DetectorGraphModel)
        self._assert_geometry(sm.ImageGraphModel)
        self._assert_geometry(sm.MultiCurveGraphModel)
        self._assert_geometry(sm.NDArrayGraphModel)
        self._assert_geometry(sm.ScatterGraphModel)
        self._assert_geometry(sm.SelectionIconsModel)
        self._assert_geometry(sm.StateGraphModel)
        self._assert_geometry(sm.TrendGraphModel)
        self._assert_geometry(sm.VectorGraphModel)
        self._assert_geometry(sm.VectorHistGraphModel)
        self._assert_geometry(sm.VectorBarGraphModel)
        self._assert_geometry(sm.VectorFillGraphModel)
        self._assert_geometry(sm.VectorRollGraphModel)
        self._assert_geometry(sm.VectorScatterGraphModel)
        self._assert_geometry(sm.VectorXYGraphModel)
        self._assert_geometry(sm.WebCamGraphModel)

    def test_icon_widgets(self):
        self._assert_geometry(sm.DigitIconsModel)
        self._assert_geometry(sm.DisplayIconsetModel, data=b"<svg></svg>")
        self._assert_geometry(sm.SelectionIconsModel)
        self._assert_geometry(sm.TextIconsModel)

    def _assert_geometry(self, model_klass, **traits):
        self._assert_single_model(model_klass, **traits)
        self._assert_valid_layout(model_klass, **traits)

        # Dead layouts mean that the children widgets do not follow the desired
        # geometry. We let Qt handle such cases, which is as such on the
        # previous implementations. There's no need to check this.
        # self._assert_dead_layout(model_klass, **traits)

    def _assert_single_model(self, model_klass, **traits):
        model = model_klass(x=10, y=10, width=100, height=30, **traits)
        self._assign_key(model)
        set_initialized_flag(model, value=True)
        expected = self._copy_model(model)

        self._set_model_to_scene(model)
        self._assert_scene_element(model, expected)

    def _assert_valid_layout(self, model_klass, **traits):
        hbox_model = self._get_layout_model(model_klass, **traits)
        expected = self._copy_model(hbox_model)

        self._set_model_to_scene(hbox_model)
        self._assert_scene_element(hbox_model, expected, modified=False)

        # Translate the widget
        self._set_offset(expected, offset=OFFSET)
        self._assert_scene_element(self._get_model(), expected,
                                   modified=True)

    def _assert_dead_layout(self, model_klass, **traits):
        """Dead layouts mean that the children widgets do not follow the
        desired geometry, with the dimensions saved smaller and the position
        always at (0, 0). We let Qt handle such cases, which is as such on the
        previous implementations. There's no need to check this.
        """
        # Initialize expected models
        expected = self._get_layout_model(model_klass, modified=True, **traits)

        # Initialize actual models. We set the position of children widgets
        # to (0, 0)
        actual = self._copy_model(expected)
        for child in actual.children:
            child.trait_setq(x=0, y=0)

        # -- Test the model
        self._set_model_to_scene(actual)
        self._assert_scene_element(actual, expected)

        # Translate the widget
        self._set_offset(expected, offset=OFFSET)
        self._assert_scene_element(self._get_model(), expected,
                                   modified=True)

    def _assert_model(self, actual, expected):
        # Check geom traits
        geom_traits = ["x", "y", "width", "height"]
        actual_values = actual.trait_get(geom_traits)
        expected_values = expected.trait_get(geom_traits)
        self.assertDictEqual(actual_values, expected_values)

    def _assert_scene_element(self, actual, expected, modified=None):
        self._assert_element(actual, expected, modified=modified)

        # Save and reread the model
        read_model = single_model_round_trip(actual)
        self._set_model_to_scene(read_model)
        self._assert_element(read_model, expected, modified=False)

        # Revert back the original model
        self._set_model_to_scene(actual)

    def _assert_element(self, actual, expected, modified=False):
        # Check model traits
        self._assert_model(actual, expected)

        # Check if model is modified
        if modified is None:
            modified = expected.modified
        self.assertEqual(actual.modified, modified)

        # Check widget
        widget = self._get_widget(actual)
        self._assert_widget(widget, model=actual)

        # Iterate over children
        if self._has_children(actual) and self._has_children(expected):
            iter_children = zip(actual.children, expected.children)
            for act_child, exp_child in iter_children:
                self._assert_element(act_child, exp_child, modified=modified)

    def _assert_widget(self, widget, model):
        scene_geom = widget.geometry()
        model_geom = model.trait_get("x", "y", "width", "height")
        self.assertEqual(scene_geom.x(), model_geom.get("x", 0))
        self.assertEqual(scene_geom.y(), model_geom.get("y", 0))
        self.assertEqual(scene_geom.width(), model_geom.get("width"))
        self.assertEqual(scene_geom.height(), model_geom.get("height"))

    def _set_model_to_scene(self, model):
        self.scene_model = sm.SceneModel(children=[model])
        with mock.patch(GET_PROXY_PATH, new=self._get_proxy):
            self.scene_view.update_model(self.scene_model)
            self.scene_view.update()
        return self.scene_model

    def _get_layout_model(self, model_klass, modified=False, **traits):
        # Initialize children models
        x0, y0 = (10, 20)
        height = 30
        label_model = sm.LabelModel(width=100, height=height, text="bar")
        display_model = sm.DisplayLabelModel(width=200, height=height)
        model = model_klass(width=300, height=height, **traits)
        children_model = [label_model, display_model, model]

        # Initialize layout model
        hbox_model = sm.BoxLayoutModel(x=x0, y=y0, children=children_model,
                                       direction=QBoxLayout.LeftToRight)

        # Correct the position from the initial position and width
        x, y = (x0, y0)
        for child in children_model:
            self._assign_key(child)
            child.x = x
            child.y = y
            # Add child width for the next iteration
            x += child.width
        hbox_model.height = height
        hbox_model.width = sum([child.width for child in children_model])
        set_initialized_flag(hbox_model, value=True)
        set_modified_flag(hbox_model, value=modified)
        return hbox_model

    def _assign_key(self, model):
        controller = get_model_controller(model)
        if controller is None:
            return None

        # Specify the binding of the flaky model
        exceptions = {sm.PopUpModel: StringBinding}
        if type(model) in exceptions:
            binding = exceptions[type(model)]
        else:
            # Use a compatible binding
            ctrait = controller.class_traits().get("_binding_type")
            binding = self._get_binding(ctrait.default_value()[1])

        model.keys = [f"{DEVICE_NAME}.{ALL_PROPS_BINDINGS[binding]}"]

    def _copy_model(self, model):
        copied = model.clone_traits(copy="deep")
        set_initialized_flag(copied, value=model.initialized)
        set_modified_flag(copied, value=model.modified)
        return copied

    def _get_widget(self, model=None):
        if model is None:
            model = self._get_model()
        return self.scene_view._scene_obj_cache.get(model)

    def _get_binding(self, bindings):
        for binding in bindings:
            if binding in ALL_PROPS_BINDINGS:
                return binding

            binding = REIMPLEMENTED_BINDINGS.get(binding)

            # We found a compatible binding
            if binding in ALL_PROPS_BINDINGS:
                return binding

    def _get_model(self):
        return self.scene_model.children[0]

    def _set_offset(self, expected, offset=(0, 0)):
        # Set the offset of the actual model
        widget = self._get_widget(self._get_model())
        widget.translate(QPoint(*offset))

        # Set the offset of the expected model
        def set_offset(model):
            model.x += offset[0]
            model.y += offset[1]

        set_offset(expected)
        if self._has_children(expected):
            for child in expected.children:
                set_offset(child)

    def _get_proxy(self, _, name):
        """Return a `PropertyProxy` instance for a given device and property
        path. This mocks the get_proxy called by ControllerContainer."""
        return self.proxy_map.get(name)

    @staticmethod
    def _has_children(model):
        return "children" in model.trait_names()
