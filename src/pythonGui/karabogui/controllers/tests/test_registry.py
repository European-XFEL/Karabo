from traits.api import Enum, Instance

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabo.middlelayer import (
    Configurable, Int8, Int16, UInt32, UInt64, String)
from karabogui.binding.api import StringBinding, IntBinding, build_binding
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


def test_known_model_collision():
    # DisplayTrendline and XYVector are known to share a scene model class
    # in a way which is unique among all the binding controllers.
    # Test that that is not a problem for the controller registry.

    from karabo.common.scenemodel.api import LinePlotModel
    from ..display.trendline import DisplayTrendline
    from ..display.xyvectors import XYVector

    model = LinePlotModel(klass='DisplayTrendline')
    controller = get_model_controller(model)
    assert controller is DisplayTrendline

    model.klass = 'XYVector'
    controller = get_model_controller(model)
    assert controller is XYVector


def test_display_edit_overlap():
    # IntLineEdit should be returned for both edit and non-edit widgets
    from karabo.common.scenemodel.api import IntLineEditModel
    from ..edit.numberlineedit import IntLineEdit

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
