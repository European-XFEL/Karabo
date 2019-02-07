from karabo.common.scenemodel.api import EditableOptionComboBoxModel
from karabo.middlelayer import Configurable, String
from karabogui.binding.api import build_binding, get_binding_value
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..optionbox import EditableOptionComboBox


class WithoutOption(Configurable):
    prop = String()


class WithOption(Configurable):
    prop = String(options=['one', 'two', 'three', 'four'])


class TestEditableOptionComboBox(GuiTestCase):

    OPTIONS = ['foo', 'bar', 'baz', 'qux']

    def setUp(self):
        super(TestEditableOptionComboBox, self).setUp()
        self.proxy = get_class_property_proxy(WithoutOption.getClassSchema(),
                                              'prop')
        model = EditableOptionComboBoxModel()
        model.options = self.OPTIONS
        self.controller = EditableOptionComboBox(proxy=self.proxy, model=model)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value_in_options(self):
        value = 'bar'
        index = self.OPTIONS.index(value)
        set_proxy_value(self.proxy, 'prop', value)

        assert self.proxy.edit_value is None
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(self.OPTIONS)
        assert self.controller._current_index == index
        assert self.controller._current_binding_value == value

    def test_set_value_not_in_options(self):
        value = 'not bar'
        new_options = self.OPTIONS + [value]
        index = new_options.index(value)
        set_proxy_value(self.proxy, 'prop', value)

        assert self.proxy.edit_value is None
        assert self.controller.model.options == new_options
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(new_options)
        assert self.controller._current_index == index
        assert self.controller._current_binding_value == value

    def test_edit_value(self):
        index = 3
        value = self.OPTIONS[index]

        self.controller._internal_widget.setCurrentIndex(index)

        assert self.proxy.edit_value == value
        assert get_binding_value(self.proxy) is None

    def test_change_model_options_with_current_value_in_options(self):
        value = "qux"
        set_proxy_value(self.proxy, 'prop', value)
        new_options = list(reversed(self.OPTIONS))
        index = new_options.index(value)
        self.controller.model.options = new_options

        assert self.proxy.edit_value is None
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(new_options)
        assert self.controller._current_index == index
        assert self.controller._current_binding_value == value

    def test_change_model_options_with_current_value_not_in_options(self):
        value = "qux"
        set_proxy_value(self.proxy, 'prop', value)
        new_options = []
        index = 0
        self.controller.model.options = new_options

        assert self.proxy.edit_value is None
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == 1  # ["qux]
        assert self.controller._current_index == index
        assert self.controller._current_binding_value == value

    def test_binding_options_injection_with_current_value_in_options(self):
        new_options = WithOption.prop.options
        index = 2
        value = new_options[index]
        set_proxy_value(self.proxy, 'prop', value)

        # inject options
        build_binding(WithOption.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)

        assert self.proxy.binding.options == new_options
        assert self.proxy.edit_value is None
        assert self.controller._current_binding_value == value
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(new_options)

    def test_binding_options_injection_with_current_value_not_in_options(self):
        value = "qux"
        set_proxy_value(self.proxy, 'prop', value)

        new_options = WithOption.prop.options + [value]
        index = new_options.index(value)

        # inject options
        build_binding(WithOption.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)

        assert self.proxy.binding.options == new_options
        assert self.proxy.edit_value is None
        assert self.controller._current_binding_value == value
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(new_options)

    def test_binding_options_removal_with_current_value(self):
        value = "qux"
        set_proxy_value(self.proxy, 'prop', value)

        # inject options
        build_binding(WithOption.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)

        options = self.controller.model.options
        index = self.controller.model.options.index(value)

        # remove options by injecting empty options
        build_binding(WithoutOption.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)

        assert self.proxy.edit_value is None
        assert self.controller._internal_widget.currentIndex() == index
        assert self.controller._internal_widget.count() == len(options)
        assert self.controller._current_index == index
        assert self.controller._current_binding_value == value
