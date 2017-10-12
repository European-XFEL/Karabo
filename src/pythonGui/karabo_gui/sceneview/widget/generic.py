import karabo_gui.gui_registry_loader  # noqa
from karabo.common.scenemodel.api import (
    AnalogModel, BitfieldModel, CheckBoxModel, ChoiceElementModel,
    ComboBoxModel, DirectoryModel, DisplayCommandModel, DisplayLabelModel,
    DisplayPlotModel, EditableListElementModel, EditableListModel,
    EditableSpinBoxModel, FileInModel, FileOutModel, HexadecimalModel,
    IntLineEditModel, KnobModel, LampModel, LineEditModel, PopUpModel,
    RunConfiguratorModel, SliderModel, XYPlotModel
)
from karabo_gui.widget import Widget

from .base import BaseWidgetContainer

_GENERIC_WIDGET_FACTORIES = {
    AnalogModel: 'DisplayAnalog',
    BitfieldModel: 'Bitfield',
    DisplayCommandModel: 'DisplayCommand',
    DisplayLabelModel: 'DisplayLabel',
    DisplayPlotModel: 'DisplayPlot',
    EditableListModel: 'EditableList',
    EditableListElementModel: 'EditableListElement',
    RunConfiguratorModel: 'RunConfiguratorEdit',
    EditableSpinBoxModel: 'EditableSpinBox',
    HexadecimalModel: 'Hexadecimal',
    IntLineEditModel: 'IntLineEdit',
    KnobModel: 'Knob',
    LampModel: 'LampWidget',
    PopUpModel: 'PopUp',
    SliderModel: 'Slider',
    XYPlotModel: 'XYPlot',
}
_GENERIC_WIDGET_FACTORIES = {k: Widget.widgets[v]
                             for k, v in _GENERIC_WIDGET_FACTORIES.items()}
_DISPLAY_EDITABLE_WIDGETS = {
    CheckBoxModel: ('DisplayCheckBox', 'EditableCheckBox'),
    ChoiceElementModel: ('DisplayChoiceElement', 'EditableChoiceElement'),
    ComboBoxModel: ('DisplayComboBox', 'EditableComboBox'),
    DirectoryModel: ('DisplayDirectory', 'EditableDirectory'),
    FileInModel: ('DisplayFileIn', 'EditableFileIn'),
    FileOutModel: ('DisplayFileOut', 'EditableFileOut'),
    LineEditModel: ('DisplayLineEdit', 'EditableLineEdit'),
}
_DISPLAY_EDITABLE_WIDGETS = {k: {n: Widget.widgets[n] for n in v}
                             for k, v in _DISPLAY_EDITABLE_WIDGETS.items()}


class GenericWidgetContainer(BaseWidgetContainer):
    """ A container for simple scene widgets which have no additional model
    data which needs to be synchronized.
    """
    def _create_widget(self, boxes):
        factory = _GENERIC_WIDGET_FACTORIES[self.model.__class__]
        widget = factory(boxes[0], self)
        for b in boxes[1:]:
            widget.addBox(b)
        return widget


class DisplayEditableWidgetContainer(BaseWidgetContainer):
    """ A container which decides whether to create a display widget or an
    editable widget depending on the model provided.
    """
    def _create_widget(self, boxes):
        factories = _DISPLAY_EDITABLE_WIDGETS[self.model.__class__]
        factory = factories[self.model.klass]
        widget = factory(boxes[0], self)
        for b in boxes[1:]:
            widget.addBox(b)
        return widget
