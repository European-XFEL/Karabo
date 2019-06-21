from unittest import mock

import numpy as np
from PyQt4 import QtGui

from ..display_vector_roll import DisplayVectorRollGraph
from karabo.common.scenemodel.api import VectorRollGraphModel
from karabo.native import Configurable, VectorInt32
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1, 2, 3])


class TestVectorRollGraph(GuiTestCase):

    def setUp(self):
        super(TestVectorRollGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayVectorRollGraph(proxy=self.proxy,
                                                 model=VectorRollGraphModel())
        with mock.patch.object(QtGui.QGraphicsTextItem, 'setHtml'):
            self.controller.create(None)
            self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestVectorRollGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        """Test the value setting in VectorRollGraph"""
        plot = self.controller._plot
        self.assertIsNotNone(plot)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, 'prop', value)
        image = self.controller._image
        np.testing.assert_almost_equal(image.data[0], value)

    def test_image_stack_configuration(self):
        controller = DisplayVectorRollGraph(proxy=self.proxy,
                                            model=VectorRollGraphModel())
        with mock.patch.object(QtGui.QGraphicsTextItem, 'setHtml'):
            controller.create(None)

        action = controller.widget.actions()[3]
        self.assertEqual(action.text(), 'Image Size')

        dsym = 'karabogui.controllers.display.display_vector_roll.QInputDialog'
        with mock.patch(dsym) as QInputDialog:
            QInputDialog.getInt.return_value = 20, True
            action.trigger()
            self.assertEqual(controller.model.maxlen, 20)
            self.assertEqual(controller._image.stack, 20)
            self.assertIsNone(controller._image.data)

        controller.destroy()
