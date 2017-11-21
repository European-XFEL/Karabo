from unittest.mock import patch, Mock

from karabo.common.scenemodel.api import SparklineModel
from karabo.middlelayer import Configurable, Float
from karabo_gui.testing import (
    GuiTestCase, get_property_proxy, get_class_property_proxy, singletons)
from .data import build_historic_data_float
from ..sparkline import DisplaySparkline


TIMEBASES = {'60s': 60, '10m': 600, '10h': 36000}


class Object(Configurable):
    prop = Float(defaultValue=0.0, warnLow=-5.0, warnHigh=5.0, alarmLow=-10.0,
                 alarmHigh=10.0)


class TestDisplaySparkline(GuiTestCase):
    def setUp(self):
        super(TestDisplaySparkline, self).setUp()
        self.schema = Object.getClassSchema()

    def test_basics(self):
        prop = get_class_property_proxy(self.schema, 'prop')

        model = SparklineModel()
        controller = DisplaySparkline(proxy=prop, model=model)
        controller.create(None)
        assert controller.widget is not None

        with patch.object(controller.line_edit, 'setVisible'):
            sym = 'karabo_gui.controllers.display.sparkline.QInputDialog'
            with patch(sym) as QInputDialog:
                for ac in controller.widget.actions():
                    ac_name = ac.text()
                    if ac_name == 'Set value format':
                        QInputDialog.getText.return_value = '.3e', True
                        ac.trigger()
                        assert controller.model.show_format == '.3e'
                    elif ac_name == 'Show value':
                        assert controller.model.show_value == ac.isChecked()
                        ac.setChecked(not ac.isChecked())
                        assert controller.model.show_value == ac.isChecked()
                    else:
                        tb_val = TIMEBASES.get(ac_name[:3], None)
                        if tb_val is None:
                            continue
                        ac.trigger()
                        assert tb_val == controller.model.time_base

        controller.destroy()
        assert controller.widget is None

    def test_device(self):
        network = Mock()
        with singletons(network=network):
            schema = Object.getClassSchema()
            val_proxy = get_property_proxy(schema, 'prop')

            model = SparklineModel()
            controller = DisplaySparkline(proxy=val_proxy, model=model)
            controller.create(None)

            val_proxy.start_monitoring()
            # can't guess the time argument because it's now
            network.onGetPropertyHistory.assert_called()

    def test_historic_data(self):
        prop = get_class_property_proxy(self.schema, 'prop')

        model = SparklineModel()
        controller = DisplaySparkline(proxy=prop, model=model)
        controller.create(None)

        prop.value = 42.0

        historic_data = build_historic_data_float()
        prop.binding.historic_data = historic_data
        # mock the QPainter, so the code path can be walked deeper down
        # without the need of creating a backend draw device
        sym = 'karabo_gui.controllers.display.sparkline.QPainter'
        with patch(sym) as QPainter:  # noqa
            controller.render_area.paintEvent(None)
        self.process_qt_events()
