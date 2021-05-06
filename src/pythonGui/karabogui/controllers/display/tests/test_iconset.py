import os.path as op
from unittest.mock import patch
from urllib.request import pathname2url

from karabo.common.scenemodel.api import DisplayIconsetModel
from karabo.native import Configurable, String
from karabogui import icons as iconspkg
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..iconset import DEFAULT_ICON_URL, DisplayIconset

TEST_ICON_PATH = op.join(op.dirname(iconspkg.__file__), 'add.svg')
TEST_ICON_URL = 'file://' + pathname2url(TEST_ICON_PATH)


class Object(Configurable):
    prop = String(defaultValue=True)


class TestDisplayIconset(GuiTestCase):
    def setUp(self):
        super(TestDisplayIconset, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.model = DisplayIconsetModel()
        self.controller = DisplayIconset(proxy=self.proxy, model=self.model)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayIconset, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_default_icon_url(self):
        assert self.model.image == DEFAULT_ICON_URL

    def test_actions(self):
        actions = self.controller.widget.actions()
        open_file_action = actions[0]
        open_url_action = actions[1]

        sym = 'karabogui.controllers.display.iconset.getOpenFileName'
        with patch(sym) as getOpenFileName:
            getOpenFileName.return_value = TEST_ICON_PATH
            open_file_action.trigger()

        assert self.model.image == TEST_ICON_URL
        self.model.image = DEFAULT_ICON_URL

        sym = 'karabogui.controllers.display.iconset.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getText.return_value = TEST_ICON_URL, True
            open_url_action.trigger()

        assert self.model.image == TEST_ICON_URL
        self.model.image = DEFAULT_ICON_URL
