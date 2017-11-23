from PyQt4.QtCore import Qt

from karabo.common.scenemodel.api import CheckBoxModel
from karabo.middlelayer import Configurable, Bool
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..checkbox import EditableCheckBox


class Object(Configurable):
    prop = Bool()


class TestEditableCheckBox(GuiTestCase):
    def setUp(self):
        super(TestEditableCheckBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableCheckBox(proxy=self.proxy,
                                           model=CheckBoxModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = True
        assert self.controller.widget.checkState() == Qt.Checked

    def test_edit_value(self):
        self.controller.widget.setCheckState(Qt.Checked)
        assert self.proxy.value

        self.controller.widget.setCheckState(Qt.Unchecked)
        assert not self.proxy.value
