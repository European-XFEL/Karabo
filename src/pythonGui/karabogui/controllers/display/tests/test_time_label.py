from unittest.mock import patch

import pytest
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import DisplayTimeModel
from karabo.native import Configurable, Float, Hash, Timestamp
from karabogui.fonts import get_font_size_from_dpi
from karabogui.testing import get_class_property_proxy, set_proxy_hash

from ..timelabel import DisplayTimeLabel


class Object(Configurable):
    prop = Float()


@pytest.mark.usefixtures("gui_app")
class TestTimeLabel:
    def setup(self):
        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, "prop")
        self.controller = DisplayTimeLabel(proxy=self.proxy,
                                           model=DisplayTimeModel())
        self.controller.create(None)

    def test_basics(self):
        controller = self.controller
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

        model = controller.model
        assert model.time_format == "%H:%M:%S"
        assert model.font_size == 10
        assert model.font_weight == "normal"

    def test_set_value(self):
        controller = self.controller
        property_hash = Hash("prop", 2.0)
        time_stamp = Timestamp("2009-04-20T10:32:22")
        set_proxy_hash(self.proxy, property_hash, time_stamp)
        assert controller.widget.text() == "10:32:22"

    def test_change_time_format(self):
        property_hash = Hash("prop", 4.0)
        time_stamp = Timestamp("2012-04-20T10:35:27")
        set_proxy_hash(self.proxy, property_hash, time_stamp)
        controller = self.controller
        action = controller.widget.actions()[0]
        assert action.text() == "Change datetime format..."

        dsym = "karabogui.controllers.display.timelabel.QInputDialog"
        with patch(dsym) as QInputDialog:
            QInputDialog.getText.return_value = "%H:%M", True
            action.trigger()
            assert controller.widget.text() == "10:35"

    def test_format_field(self):
        """
        Test contex menu-item opens the dialog and the model values are
        set from the dialog.
        """
        controller = self.controller
        action = controller.widget.actions()[1]
        assert action.text() == "Format field..."

        path = "karabogui.controllers.display.timelabel.FormatLabelDialog"
        with patch(path) as dialog:
            dialog().exec.return_value = QDialog.Accepted
            dialog().font_size = 11
            dialog().font_weight = "bold"
            action.trigger()
            assert dialog().exec.call_count == 1
            assert controller.model.font_size == 11
            assert controller.model.font_weight == "bold"

    def test_apply_format(self):
        """
        Test '_apply_format' applies the font size and wieght to widget from
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
