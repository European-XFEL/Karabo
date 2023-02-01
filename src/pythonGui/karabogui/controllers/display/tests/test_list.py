from unittest.mock import patch

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import DisplayListModel
from karabo.native import Configurable, VectorInt32
from karabogui.binding.api import apply_default_configuration
from karabogui.controllers.display.list import DisplayList
from karabogui.fonts import get_font_size_from_dpi
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


class TestDisplayList(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
        self.controller = DisplayList(proxy=self.proxy,
                                      model=DisplayListModel())
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.NoFocus

    def test_set_value(self):
        set_proxy_value(self.proxy, "prop", [0, 2])
        assert self.controller.widget.text() == "0,2"

    def test_default_values(self):
        model = self.controller.model
        assert model.font_size == 10
        assert model.font_weight == "normal"

    def test_format_field(self):
        """
        Test contex menu-item opens the dialog and the model values are
        set from the dialog.
        """
        controller = self.controller
        action = controller.widget.actions()[0]
        assert action.text() == "Format field..."

        path = "karabogui.controllers.display.list.FormatLabelDialog"
        with patch(path) as dialog:
            dialog().exec.return_value = QDialog.Accepted
            dialog().font_size = 12
            dialog().font_weight = "bold"
            action.trigger()
            assert dialog().exec.call_count == 1
            assert controller.model.font_size == 12
            assert controller.model.font_weight == "bold"

    def test_apply_format(self):
        """
        Test '_apply_format' applies the font size and weight to widget from
        the model.
        """
        controller = self.controller
        widget = controller.widget
        controller._apply_format(widget)
        font = widget.font()
        size = font.pointSize()
        bold = font.bold()

        assert size == get_font_size_from_dpi(controller.model.font_size)
        assert bold == (controller.model.font_weight == "bold")
