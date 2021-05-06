from unittest.mock import patch

from karabo.native import Configurable, String
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..filesystem import EditableDirectory, EditableFileIn, EditableFileOut


class Object(Configurable):
    prop = String(displayType='directory')


class TestEditFilesystemControllers(GuiTestCase):
    def setUp(self):
        super(TestEditFilesystemControllers, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')

    def test_directory(self):
        controller = EditableDirectory(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        sym = 'karabogui.controllers.edit.filesystem.QFileDialog'
        with patch(sym) as QFileDialog:
            QFileDialog.getExistingDirectory.return_value = '/karabo'
            controller._on_button_click()
            assert self.proxy.edit_value == '/karabo'

        controller.destroy()
        assert controller.widget is None

    def test_filein(self):
        controller = EditableFileIn(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        sym = 'karabogui.controllers.edit.filesystem.getOpenFileName'
        with patch(sym) as getOpenFileName:
            getOpenFileName.return_value = 'file.txt'
            controller._on_button_click()
            assert self.proxy.edit_value == 'file.txt'

        controller.destroy()
        assert controller.widget is None

    def test_fileout(self):
        controller = EditableFileOut(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        sym = 'karabogui.controllers.edit.filesystem.getSaveFileName'
        with patch(sym) as getSaveFileName:
            getSaveFileName.return_value = 'file.txt'
            controller._on_button_click()
            assert self.proxy.edit_value == 'file.txt'

        controller.destroy()
        assert controller.widget is None
