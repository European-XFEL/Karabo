import karabo_gui.gui_registry_loader  # noqa
from karabo_gui.scenemodel.api import (
    BitfieldModel, DisplayAlignedImageModel, DisplayCommandModel,
    DisplayIconsetModel, DisplayImageModel, DisplayImageElementModel,
    DisplayLabelModel, DisplayPlotModel, DoubleLineEditModel,
    EditableListModel, EditableListElementModel, EditableSpinBoxModel,
    HexadecimalModel, IntLineEditModel, KnobModel, SliderModel, XYPlotModel
)
from karabo_gui.widget import Widget
from .base import BaseWidgetContainer

_GENERIC_WIDGET_FACTORIES = {
    BitfieldModel: 'Bitfield',
    DisplayAlignedImageModel: 'DisplayAlignedImage',
    DisplayCommandModel: 'DisplayCommand',
    DisplayIconsetModel: 'DisplayIconset',
    DisplayImageModel: 'DisplayImage',
    DisplayImageElementModel: 'DisplayImageElement',
    DisplayLabelModel: 'DisplayLabel',
    DisplayPlotModel: 'DisplayPlot',
    DoubleLineEditModel: 'DoubleLineEdit',
    EditableListModel: 'EditableList',
    EditableListElementModel: 'EditableListElement',
    EditableSpinBoxModel: 'EditableSpinBox',
    HexadecimalModel: 'Hexadecimal',
    IntLineEditModel: 'IntLineEdit',
    KnobModel: 'Knob',
    SliderModel: 'Slider',
    XYPlotModel: 'XYPlot',
}
_GENERIC_WIDGET_FACTORIES = {k: Widget.widgets[v]
                             for k, v in _GENERIC_WIDGET_FACTORIES.items()}


class GenericWidgetContainer(BaseWidgetContainer):
    """ A container for simple scene widgets which have no additional model
    data which needs to be synchronized.
    """
    def _create_widget(self, boxes):
        factory = _GENERIC_WIDGET_FACTORIES[self.model.__class__]
        display_widget = factory(boxes[0], self)
        for b in boxes[1:]:
            display_widget.addBox(b)
        return display_widget.widget
