# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest.mock import patch

from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import TickSliderModel
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..slider import TickSlider
from .utils import InRangeInt, LargeRange, Object, Other


class TestSlider(GuiTestCase):
    def setUp(self):
        super(TestSlider, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = TickSlider(proxy=self.proxy, model=TickSliderModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_focus_policy(self):
        assert self.controller.slider.focusPolicy() == Qt.StrongFocus

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 1.0)
        self.assertEqual(self.controller.slider.value(), 1.0)
        self.assertEqual(self.controller.label.text(), "1.0")
        set_proxy_value(self.proxy, 'prop', 1.3)
        self.assertEqual(self.controller.slider.value(), 1.0)
        self.assertEqual(self.controller.label.text(), "1.3")
        set_proxy_value(self.proxy, 'prop', 1.9)
        self.assertEqual(self.controller.slider.value(), 1.0)
        self.assertEqual(self.controller.label.text(), "1.9")

    def test_edit_value(self):
        self.controller.slider.valueChanged.emit(3)
        self.assertEqual(self.proxy.edit_value, 3)
        self.assertEqual(self.controller.label.text(), "3.0")

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = TickSlider(proxy=proxy)
        controller.create(None)

        self.assertEqual(controller.slider.minimum(), 1)
        self.assertEqual(controller.slider.maximum(), 4)

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        self.assertEqual(controller.slider.minimum(), -2.0)
        self.assertEqual(controller.slider.maximum(), 4.0)

    def test_actions(self):
        controller = TickSlider(proxy=self.proxy,
                                model=TickSliderModel())
        controller.create(None)
        action = controller.widget.actions()[0]
        self.assertEqual(action.text(), 'Tick Interval')

        dsym = 'karabogui.controllers.edit.slider.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getInt.return_value = 20, True
            action.trigger()
            self.assertEqual(controller.model.ticks, 20)
            self.assertEqual(controller.slider.tickInterval(), 20)
            self.assertEqual(controller.slider.singleStep(), 20)

        action = controller.widget.actions()[1]
        self.assertEqual(action.text(), 'Show value')
        self.assertTrue(controller.model.show_value)
        action.trigger()
        self.assertFalse(controller.model.show_value)

        controller.destroy()

    def test_large_range(self):
        proxy = get_class_property_proxy(LargeRange.getClassSchema(), 'prop')
        controller = TickSlider(proxy=proxy)
        controller.create(None)

        assert not controller.widget.isEnabled()

    def test_large_int(self):
        proxy = get_class_property_proxy(InRangeInt.getClassSchema(), 'prop')
        controller = TickSlider(proxy=proxy)
        controller.create(None)

        assert controller.widget.isEnabled()
