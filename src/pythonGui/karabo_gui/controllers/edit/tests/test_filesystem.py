from unittest.mock import patch

from karabo.middlelayer import Configurable, String
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
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

        sym = 'karabo_gui.controllers.edit.filesystem.QFileDialog'
        with patch(sym) as QFileDialog:
            QFileDialog.getExistingDirectory.return_value = '/karabo'
            controller._on_button_click()
            assert self.proxy.value == '/karabo'

        controller.destroy()
        assert controller.widget is None

    def test_filein(self):
        controller = EditableFileIn(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        sym = 'karabo_gui.controllers.edit.filesystem.getOpenFileName'
        with patch(sym) as getOpenFileName:
            getOpenFileName.return_value = 'file.txt'
            controller._on_button_click()
            assert self.proxy.value == 'file.txt'

        controller.destroy()
        assert controller.widget is None

    def test_fileout(self):
        controller = EditableFileOut(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        sym = 'karabo_gui.controllers.edit.filesystem.getSaveFileName'
        with patch(sym) as getSaveFileName:
            getSaveFileName.return_value = 'file.txt'
            controller._on_button_click()
            assert self.proxy.value == 'file.txt'

        controller.destroy()
        assert controller.widget is None
