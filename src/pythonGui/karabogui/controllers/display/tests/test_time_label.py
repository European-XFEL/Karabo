from unittest.mock import patch

from karabo.common.scenemodel.api import DisplayTimeModel
from karabo.native import Configurable, Float, Hash, Timestamp
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_hash)

from ..timelabel import DisplayTimeLabel


class Object(Configurable):
    prop = Float()


class TestTimeLabel(GuiTestCase):
    def setUp(self):
        super(TestTimeLabel, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.t1 = Timestamp("2009-04-20T10:32:22")
        self.t2 = Timestamp("2012-04-20T10:35:27")

    def test_basics(self):
        controller = DisplayTimeLabel(proxy=self.proxy,
                                      model=DisplayTimeModel())
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        model = DisplayTimeModel(time_format='%H:%M:%S')
        controller = DisplayTimeLabel(proxy=self.proxy, model=model)
        controller.create(None)
        h = Hash('prop', 2.0)
        set_proxy_hash(self.proxy, h, self.t1)
        self.assertEqual(controller.widget.text(), '10:32:22')

    def test_change_time_format(self):
        h = Hash('prop', 4.0)
        set_proxy_hash(self.proxy, h, self.t2)
        controller = DisplayTimeLabel(proxy=self.proxy,
                                      model=DisplayTimeModel())
        controller.create(None)
        action = controller.widget.actions()[0]
        assert action.text() == 'Change datetime format...'

        dsym = 'karabogui.controllers.display.timelabel.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getText.return_value = '%H:%M', True
            action.trigger()
            self.assertEqual(controller.widget.text(), '10:35')
