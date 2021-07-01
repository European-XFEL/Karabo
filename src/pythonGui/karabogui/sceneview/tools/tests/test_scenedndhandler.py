from unittest.mock import patch

from qtpy.QtCore import QEvent, QPoint, Qt
from qtpy.QtGui import QDropEvent

from karabo.common.scenemodel.api import BoxLayoutModel, SceneModel
from karabo.common.states import State
from karabo.native import AccessMode, Configurable, Double, String
from karabogui.configurator.utils import dragged_configurator_items
from karabogui.sceneview.tools.scenedndhandler import ConfigurationDropHandler
from karabogui.sceneview.view import SceneView
from karabogui.testing import GuiTestCase, get_property_proxy

GET_PROXY_PATH = 'karabogui.sceneview.tools.scenedndhandler.get_proxy'


class TypeObject(Configurable):
    state = String(
        defaultValue=State.ON,
        enum=State,
        displayType="State",
        accessMode=AccessMode.READONLY)
    prop = Double(
        minInc=-1000.0,
        maxInc=1000.0,
        allowedStates=[State.ON])


class TestSceneDnDHandler(GuiTestCase):
    def setUp(self):
        super().setUp()
        schema = TypeObject.getClassSchema()
        self.proxy = get_property_proxy(schema, 'prop')
        self.state_proxy = get_property_proxy(schema, 'state')
        self.assertIsNotNone(self.proxy.binding)
        self.assertIsNotNone(self.state_proxy.binding)
        self.scene_model = SceneModel(children=[])
        self.scene_view = SceneView(model=self.scene_model)

    def tearDown(self):
        super().tearDown()
        self.scene_view.destroy()
        self.scene_view = None
        self.scene_model = None

    def test_configuration_drop_handler(self):
        # 1. Reconfigurable item
        items = dragged_configurator_items([self.proxy])
        self.assertIsNotNone(items)

        event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                           Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
        handler = ConfigurationDropHandler()
        self.assertTrue(handler.can_handle(event))

        self.assertEqual(len(self.scene_model.children), 0)

        def get_proxy(*args, **kwargs):
            return self.proxy

        with patch(GET_PROXY_PATH, new=get_proxy):
            handler.handle(self.scene_view, event)

        self.assertEqual(len(self.scene_model.children), 1)
        self.assertIsInstance(self.scene_model.children[0], BoxLayoutModel)

        # 2. Readonly param
        items = dragged_configurator_items([self.state_proxy])
        self.assertIsNotNone(items)

        event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                           Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
        self.assertTrue(handler.can_handle(event))

        self.assertEqual(len(self.scene_model.children), 1)

        def get_proxy(*args, **kwargs):
            return self.state_proxy

        with patch(GET_PROXY_PATH, new=get_proxy):
            handler.handle(self.scene_view, event)

        self.assertEqual(len(self.scene_model.children), 2)
        self.assertIsInstance(self.scene_model.children[1], BoxLayoutModel)
