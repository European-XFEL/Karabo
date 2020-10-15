#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op
from unittest import mock

from PyQt5.QtCore import QPoint
from PyQt5.QtWidgets import QBoxLayout

from karabo.common.api import ProxyStatus, set_initialized_flag
from karabo.common.scenemodel.api import (
    read_scene, BoxLayoutModel, DisplayLabelModel, LabelModel, SceneModel,
    UnknownXMLDataModel)
import karabo.common.scenemodel.tests as sm
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabo.native import Configurable, Int32

from karabogui.binding.api import build_binding, DeviceProxy, PropertyProxy
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


OFFSET = (10, 20)
GET_PROXY_PATH = "karabogui.sceneview.widget.container.get_proxy"
DEVICE_NAME = "Device"


class Object(Configurable):
    int32Property = Int32()


class TestLoadSceneModel(GuiTestCase):

    def setUp(self):
        super(TestLoadSceneModel, self).setUp()
        # Setup device
        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceProxy(binding=binding,
                             server_id='Fake',
                             device_id=DEVICE_NAME,
                             status=ProxyStatus.OFFLINE)

        self.proxy_map = {name: PropertyProxy(root_proxy=device, path=name)
                          for name in ["int32Property"]}

        # Setup scene view
        self.view = SceneView()
        self.view.show()
        self.scene_model = SceneModel()

    def tearDown(self):
        super(TestLoadSceneModel, self).setUp()
        self.view.close()
        self.view.destroy()
        self.view = None
        self.scene_model = None

    def test_basic(self):
        self.view.update_model(self.scene_model)
        scene_geom = self.view.geometry()
        self.assertEqual(scene_geom.width(), self.scene_model.width)
        self.assertEqual(scene_geom.height(), self.scene_model.height)

    def test_text_widgets(self):
        label_model = LabelModel(x=10, y=10, width=100, height=30, text="bar")
        copied_traits = label_model.clone_traits(copy="deep")
        self._set_model_to_scene(label_model)
        self._assert_scene_element(label_model, copied_traits)

    def test_layouts(self):
        # Initialize children models
        property_name = f"{DEVICE_NAME}.int32Property"
        label_model = LabelModel(x=10, y=10, width=100, height=30, text="bar")
        display_model = DisplayLabelModel(x=110, y=10, width=100, height=30,
                                          keys=[property_name])

        # Initialize model
        hbox_model = BoxLayoutModel(x=10, y=10, width=200, height=30,
                                    direction=QBoxLayout.LeftToRight,
                                    children=[label_model, display_model])
        set_initialized_flag(hbox_model, value=True)

        # Clone the model and set to initialize again
        expected_model = hbox_model.clone_traits(copy="deep")
        set_initialized_flag(expected_model, value=True)
        self._set_model_to_scene(hbox_model)
        self._assert_scene_element(hbox_model, expected_model)

        # Translate the widget
        self._set_offset(expected_model, offset=OFFSET)
        self._assert_scene_element(self._get_model(), expected_model)

    def _assert_model(self, actual, expected):
        # Check geom traits
        geom_traits = ["x", "y", "width", "height"]
        actual_values = actual.trait_get(geom_traits)
        expected_values = expected.trait_get(geom_traits)
        self.assertDictEqual(actual_values, expected_values)

    def _assert_scene_element(self, actual, expected):
        self._assert_element(actual, expected)

        # Save and reread the model
        read_model = single_model_round_trip(actual)
        self._set_model_to_scene(read_model)
        self._assert_element(read_model, expected, modified=False)

        # Revert back the original model
        self._set_model_to_scene(actual)

    def _assert_element(self, actual, expected, modified=None):
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
        self.scene_model = SceneModel(children=[model])
        with mock.patch(GET_PROXY_PATH, new=self._get_proxy):
            self.view.update_model(self.scene_model)
            self.view.update()
        return self.scene_model

    def _get_widget(self, model=None):
        if model is None:
            model = self._get_model()
        return self.view._scene_obj_cache.get(model)

    def _get_model(self):
        return self.scene_model.children[0]

    def _set_offset(self, model, offset=(0, 0)):
        widget = self._get_widget(self._get_model())
        widget.translate(QPoint(*offset))

        def set_offset(model):
            model.x += offset[0]
            model.y += offset[1]

        set_offset(model)
        if self._has_children(model):
            for child in model.children:
                set_offset(child)

    def _get_proxy(self, _, name):
        """Return a `PropertyProxy` instance for a given device and property
        path. This mocks the get_proxy called by ControllerContainer."""
        return self.proxy_map.get(name)

    @staticmethod
    def _has_children(model):
        return "children" in model.trait_names()
