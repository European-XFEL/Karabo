from unittest.mock import patch

from PyQt4.QtCore import QByteArray, Qt
from PyQt4.QtGui import QDialog, QWidget

from karabo.common.api import State
from karabo.common.scenemodel.api import StatefulIconWidgetModel
from karabo.native import Configurable, String
from karabogui.indicators import STATE_COLORS
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..statefulicon import StatefulIconWidget

ICON_NAME = 'icon_bdump'


class MockSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class MockDialog(QDialog):
    singleton = None

    def exec_(self):
        # Yeah... We need to break into the widget's code
        MockDialog.singleton = self


class Object(Configurable):
    state = String(displayType='State')


class TestStatefulIconWidget(GuiTestCase):
    def setUp(self):
        super(TestStatefulIconWidget, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'state')
        self.model = StatefulIconWidgetModel(icon_name=ICON_NAME)

    def test_basics(self):
        controller = StatefulIconWidget(proxy=self.proxy, model=self.model)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        target = 'karabogui.controllers.display.statefulicon.QSvgWidget'
        with patch(target, new=MockSvgWidget):
            controller = StatefulIconWidget(proxy=self.proxy, model=self.model)
            controller.create(None)

            states = ('CHANGING', 'ACTIVE', 'PASSIVE', 'DISABLED', 'STATIC',
                      'RUNNING', 'NORMAL', 'ERROR', 'INIT', 'UNKNOWN')

            for state in states:
                set_proxy_value(self.proxy, 'state', state)
                color = STATE_COLORS[getattr(State, state)]
                svg = controller._icon.with_color(color)
                assert controller.widget.loaded_data == QByteArray(svg)

    def test_pick_icon(self):
        target = 'karabogui.controllers.display.statefulicon.QDialog'
        with patch(target, new=MockDialog):
            model = StatefulIconWidgetModel()
            controller = StatefulIconWidget(proxy=self.proxy, model=model)
            controller.create(None)

            # XXX: Really dig around inside the brains of the dialog opened
            # by controller._show_icon_picker.
            assert MockDialog.singleton is not None
            iconlist = MockDialog.singleton.children()[0]  # QListView
            item_model = iconlist.model()  # QStandardItemModel
            item_index = item_model.index(5, 0)  # QModelIndex
            # Mimic the user double clicking
            iconlist.doubleClicked.emit(item_index)

            # Test that picking an icon actually set it to the scene model
            icon = item_index.data(Qt.UserRole + 1)  # QIcon
            assert controller._icon is icon
            assert model.icon_name == icon.name
