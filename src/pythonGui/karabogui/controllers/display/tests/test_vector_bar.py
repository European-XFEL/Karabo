from unittest.mock import patch

from ..display_vector_bar import DisplayBarGraph
from karabo.common.scenemodel.api import VectorBarGraphModel

from karabo.native import Configurable, VectorInt32
from karabogui.testing import (GuiTestCase, get_class_property_proxy,
                               set_proxy_value)


class Object(Configurable):
    value = VectorInt32()


class TestDisplayVectorBar(GuiTestCase):
    def setUp(self):
        super(TestDisplayVectorBar, self).setUp()

        schema = Object.getClassSchema()
        self.value = get_class_property_proxy(schema, 'value')
        self.controller = DisplayBarGraph(proxy=self.value)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestDisplayVectorBar, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_bar_graph_basics(self):
        value = [2, 4, 6]
        set_proxy_value(self.value, 'value', value)
        curve = self.controller._plot
        self.assertEqual(list(curve.opts.get('x')), [0, 1, 2])
        self.assertEqual(list(curve.opts.get('height')), value)

    def test_bar_width(self):
        controller = DisplayBarGraph(proxy=self.value,
                                     model=VectorBarGraphModel())
        controller.create(None)
        action = controller.widget.actions()[8]
        self.assertEqual(action.text(), 'Bar Width')

        dsym = 'karabogui.controllers.display.display_vector_bar.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getDouble.return_value = 2.7, True
            action.trigger()
            self.assertEqual(controller.model.bar_width, 2.7)
            self.assertEqual(controller._plot.opts['width'], 2.7)

        controller.destroy()
