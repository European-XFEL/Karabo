import karabo_gui.gui_registry_loader  # noqa
from karabo.common.scenemodel import api as models
from karabo_gui.widget import Widget
from .base import BaseWidgetContainer

_GENERIC_WIDGET_FACTORIES = {
    models.AnalogModel: 'DisplayAnalog',
    models.BitfieldModel: 'Bitfield',
    models.DisplayCommandModel: 'DisplayCommand',
    models.DisplayLabelModel: 'DisplayLabel',
    models.DisplayPlotModel: 'DisplayPlot',
    models.DisplayTextLogModel: 'DisplayTextLog',
    models.EditableListModel: 'EditableList',
    models.EditableListElementModel: 'EditableListElement',
    models.RunConfiguratorModel: 'RunConfiguratorEdit',
    models.EditableSpinBoxModel: 'EditableSpinBox',
    models.HexadecimalModel: 'Hexadecimal',
    models.IntLineEditModel: 'IntLineEdit',
    models.KnobModel: 'Knob',
    models.LampModel: 'LampWidget',
    models.PopUpModel: 'PopUp',
    models.SliderModel: 'Slider',
    models.XYPlotModel: 'XYPlot',
}
_GENERIC_WIDGET_FACTORIES = {k: Widget.widgets[v]
                             for k, v in _GENERIC_WIDGET_FACTORIES.items()}
_DISPLAY_EDITABLE_WIDGETS = {
    models.CheckBoxModel: ('DisplayCheckBox', 'EditableCheckBox'),
    models.ChoiceElementModel: ('DisplayChoiceElement',
                                'EditableChoiceElement'),
    models.ComboBoxModel: ('DisplayComboBox', 'EditableComboBox'),
    models.DirectoryModel: ('DisplayDirectory', 'EditableDirectory'),
    models.FileInModel: ('DisplayFileIn', 'EditableFileIn'),
    models.FileOutModel: ('DisplayFileOut', 'EditableFileOut'),
    models.LineEditModel: ('DisplayLineEdit', 'EditableLineEdit'),
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
