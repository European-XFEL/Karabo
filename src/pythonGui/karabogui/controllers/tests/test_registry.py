# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from traits.api import Enum, Instance

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabo.native import Configurable, Int8, Int16, String, UInt32, UInt64
from karabogui.binding.api import IntBinding, StringBinding, build_binding
from karabogui.testing import GuiTestCase, flushed_registry

from ..api import (
    BaseBindingController, get_class_const_trait, get_compatible_controllers,
    get_model_controller, register_binding_controller)


class SomeObject(Configurable):
    for_display = String(options=['foo', 'bar', 'baz'])
    for_editing = String()
    unsupported = Int8()


class Integers(Configurable):
    int8 = Int8()
    int16 = Int16()
    uint32 = UInt32()
    uint64 = UInt64()


class UniqueWidgetModel(BaseWidgetObjectData):
    pass  # Satisfy the uniqueness check in register_binding_controller


def _options_checker(binding):
    return len(binding.options) > 0


def test_registry():
    with flushed_registry():
        @register_binding_controller(klassname='Editor',
                                     binding_type=StringBinding)
        class DisplayWidget(BaseBindingController):
            model = Instance(UniqueWidgetModel)

        @register_binding_controller(binding_type=StringBinding,
                                     klassname='Displayer',
                                     is_compatible=_options_checker)
        class DisplayWidgetOptions(BaseBindingController):
            model = Instance(UniqueWidgetModel)

        @register_binding_controller(klassname='Editor',
                                     binding_type=StringBinding, can_edit=True)
        class EditWidget(BaseBindingController):
            model = Instance(UniqueWidgetModel)

        schema = SomeObject.getClassSchema()
        binding = build_binding(schema)

        bind = binding.value.for_display
        for_display_widgets = get_compatible_controllers(bind)
        assert DisplayWidget in for_display_widgets
        assert DisplayWidgetOptions in for_display_widgets
        assert EditWidget not in for_display_widgets

        bind = binding.value.for_editing
        for_editing_widgets = get_compatible_controllers(bind, can_edit=True)
        assert DisplayWidget not in for_editing_widgets
        assert DisplayWidgetOptions not in for_editing_widgets
        assert EditWidget in for_editing_widgets
        for_editing_widgets = get_compatible_controllers(bind)
        assert DisplayWidget in for_editing_widgets
        assert DisplayWidgetOptions not in for_editing_widgets
        assert EditWidget not in for_editing_widgets

        bind = binding.value.unsupported
        assert get_compatible_controllers(bind) == []


def test_expanded_registry():
    with flushed_registry():
        @register_binding_controller(klassname='Displayer',
                                     binding_type=IntBinding)
        class DisplayWidget(BaseBindingController):
            model = Instance(UniqueWidgetModel)

        schema = Integers.getClassSchema()
        binding = build_binding(schema)

        for name in ('int8', 'int16', 'uint32', 'uint64'):
            bind = getattr(binding.value, name)
            assert DisplayWidget in get_compatible_controllers(bind)


def test_scene_model_registry():
    class DisplayModel(BaseWidgetObjectData):
        pass

    class EditModel(BaseWidgetObjectData):
        def _parent_component_default(self):
            return 'EditableApplyLaterComponent'

    class SharedModel(BaseWidgetObjectData):
        klass = Enum('Foo', 'Bar')

    class UnusedModel(BaseWidgetObjectData):
        pass

    with flushed_registry():
        @register_binding_controller(klassname='Displayer',
                                     binding_type=StringBinding)
        class DisplayWidget(BaseBindingController):
            model = Instance(DisplayModel)

        @register_binding_controller(klassname='Editor',
                                     binding_type=StringBinding, can_edit=True)
        class EditWidget(BaseBindingController):
            model = Instance(EditModel)

        @register_binding_controller(klassname='Bar',
                                     binding_type=StringBinding)
        class BarWidget(BaseBindingController):
            model = Instance(SharedModel)

        @register_binding_controller(klassname='Foo',
                                     binding_type=StringBinding)
        class FooWidget(BaseBindingController):
            model = Instance(SharedModel)

        controller = get_model_controller(DisplayModel())
        assert controller is DisplayWidget

        controller = get_model_controller(EditModel())
        assert controller is EditWidget

        model = SharedModel(klass='Foo')
        controller = get_model_controller(model)
        assert controller is FooWidget

        model.klass = 'Bar'
        controller = get_model_controller(model)
        assert controller is BarWidget

        model = UnusedModel()
        assert get_model_controller(model) is None


def test_display_edit_overlap():
    # IntLineEdit should be returned for both edit and non-edit widgets
    from karabo.common.scenemodel.api import IntLineEditModel

    from ..edit.lineedit import IntLineEdit

    model = IntLineEditModel(parent_component='DisplayComponent')
    assert get_model_controller(model) is IntLineEdit

    model = IntLineEditModel(
        parent_component='EditableApplyLaterComponent')
    assert get_model_controller(model) is IntLineEdit


# Use a GuiTestCase to ensure a populated controller registry
class TestEditableControllerModels(GuiTestCase):
    def test_models_are_editable(self):
        from karabogui.controllers import registry

        editable_controllers = set()
        for klasses in registry._controller_registry.values():
            for klass in klasses:
                if get_class_const_trait(klass, '_can_edit'):
                    editable_controllers.add(klass)

        for klass in editable_controllers:
            modelklass = klass.class_traits()['model'].trait_type.klass
            has_klass = 'klass' in modelklass.class_traits()
            if not has_klass:
                assert modelklass().parent_component != 'DisplayComponent'

    def test_models_are_valid(self):
        from karabogui.controllers import registry

        for klasses in registry._controller_registry.values():
            for klass in klasses:
                assert klass().model is not None
