# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from platform import system
from unittest import mock, skipIf

import numpy as np
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.common.scenemodel.api import VectorRollGraphModel
from karabo.native import (
    Configurable, Hash, Int32, NDArray, Timestamp, VectorInt32)
from karabogui.graph.common.api import AuxPlots
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_hash, set_proxy_value)

from ..vector_roll_graph import ArrayRollGraph


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1, 2, 3])


class NDArrayObject(Configurable):
    prop = NDArray(
        defaultValue=np.arange(10, dtype=np.int32),
        shape=(10,),
        dtype=Int32)


class TestVectorRollGraph(GuiTestCase):

    def setUp(self):
        super(TestVectorRollGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = ArrayRollGraph(proxy=self.proxy,
                                         model=VectorRollGraphModel())

        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            self.controller.create(None)
            self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestVectorRollGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_configuration(self):
        """Assert that the vector roll auxiliar plots do not smooth the
        images"""
        aux_plots = self.controller.widget._aux_plots
        controller = aux_plots._aggregators[AuxPlots.ProfilePlot]
        self.assertFalse(controller.smooth)

    @skipIf(system() == "Windows",
            reason="image.data is None in Windows tests")
    def test_set_value(self):
        """Test the value setting in VectorRollGraph"""
        plot = self.controller._plot
        self.assertIsNotNone(plot)
        value = [2, 4, 6]
        set_proxy_value(self.proxy, 'prop', value)
        image = self.controller._image
        np.testing.assert_almost_equal(image.data[0], value)

    @skipIf(system() == "Windows",
            reason="image.data is None in Windows tests")
    def test_set_value_timestamp(self):
        """Test the value setting with same timestamp in VectorRollGraph"""
        plot = self.controller._plot
        self.assertIsNotNone(plot)
        timestamp = Timestamp()
        h = Hash('prop', [2, 4, 6])
        set_proxy_hash(self.proxy, h, timestamp)
        image = self.controller._image
        np.testing.assert_almost_equal(image.data[0], [2, 4, 6])
        h = Hash('prop', [22, 34, 16])
        set_proxy_hash(self.proxy, h, timestamp)
        image = self.controller._image
        np.testing.assert_almost_equal(image.data[0], [2, 4, 6])

    def test_image_stack_configuration(self):
        controller = ArrayRollGraph(proxy=self.proxy,
                                    model=VectorRollGraphModel())
        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            controller.create(None)

        action = controller.widget.actions()[3]
        self.assertEqual(action.text(), 'Image Size')

        dsym = 'karabogui.controllers.display.vector_roll_graph.QInputDialog'
        with mock.patch(dsym) as QInputDialog:
            QInputDialog.getInt.return_value = 20, True
            action.trigger()
            self.assertEqual(controller.model.maxlen, 20)
            self.assertEqual(controller._image.stack, 20)
            self.assertIsNone(controller._image.data)

        controller.destroy()


class TestArrayRollGraph(GuiTestCase):
    def setUp(self):
        super(TestArrayRollGraph, self).setUp()
        schema = NDArrayObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = ArrayRollGraph(proxy=self.proxy)
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestArrayRollGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @skipIf(system() == "Windows",
            reason="image.data is None in Windows tests")
    def test_set_value(self):
        value = np.array(
            [2, 3, 2, 3, 2, 3, 2, 3, 2, 3],
            dtype=np.int32)
        array_hash = Hash('type', 12,
                          'data', value.tobytes())
        set_proxy_value(self.proxy, 'prop', array_hash)

        image = self.controller._image
        np.testing.assert_almost_equal(image.data[0], value)
