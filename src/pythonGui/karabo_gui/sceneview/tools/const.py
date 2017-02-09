from karabo.common.scenemodel.api import (
    BitfieldModel, CheckBoxModel, ChoiceElementModel, ComboBoxModel,
    DigitIconsModel, DirectoryModel, DisplayAlignedImageModel,
    DisplayCommandModel, DisplayIconsetModel, DisplayImageElementModel,
    DisplayImageModel, DisplayLabelModel, DisplayPlotModel,
    DisplayStateColorModel, DoubleLineEditModel, EditableListElementModel,
    EditableListModel, EditableSpinBoxModel, EvaluatorModel, FileInModel,
    FileOutModel, FloatSpinBoxModel, HexadecimalModel, IntLineEditModel,
    KnobModel, LampModel, LineEditModel, LinePlotModel, MonitorModel,
    PopUpModel, ScientificImageModel, SelectionIconsModel, SingleBitModel,
    SliderModel, TableElementModel, TextIconsModel, WebcamImageModel,
    XYPlotModel, StatefulIconWidgetModel
)


# This is a mapping of Widget class names -> scene model classes
# It is used in situations where ``Widget`` is asked for a list of classes
# which can work together with a ``Box`` object. The class name is used to
# fetch a scene model which can be added to the scene which will result in a
# widget which works with a box.
# Examples:
# * Switching widgets for a device property
# * Adding a default widget when a property is dropped on the scene
WIDGET_FACTORIES = {
    'Bitfield': BitfieldModel,
    'DisplayCheckBox': CheckBoxModel,
    'EditableCheckBox': CheckBoxModel,
    'DisplayChoiceElement': ChoiceElementModel,
    'EditableChoiceElement': ChoiceElementModel,
    'DisplayComboBox': ComboBoxModel,
    'EditableComboBox': ComboBoxModel,
    'DisplayCommand': DisplayCommandModel,
    'DisplayDirectory': DirectoryModel,
    'EditableDirectory': DirectoryModel,
    'DisplayFileIn': FileInModel,
    'EditableFileIn': FileInModel,
    'DisplayFileOut': FileOutModel,
    'EditableFileOut': FileOutModel,
    'DisplayIconset': DisplayIconsetModel,
    'DisplayAlignedImage': DisplayAlignedImageModel,
    'DisplayImage': DisplayImageModel,
    'DisplayImageElement': DisplayImageElementModel,
    'ScientificImageDisplay': ScientificImageModel,
    'WebcamImageDisplay': WebcamImageModel,
    'DisplayLabel': DisplayLabelModel,
    'DisplayLineEdit': LineEditModel,
    'EditableLineEdit': LineEditModel,
    'EditableList': EditableListModel,
    'EditableListElement': EditableListElementModel,
    'DisplayPlot': DisplayPlotModel,
    'EditableSpinBox': EditableSpinBoxModel,
    'Hexadecimal': HexadecimalModel,
    'DoubleLineEdit': DoubleLineEditModel,
    'IntLineEdit': IntLineEditModel,
    'Slider': SliderModel,
    'Knob': KnobModel,
    'LampWidget': LampModel,
    'XYPlot': XYPlotModel,
    'XYVector': LinePlotModel,
    'DisplayTrendline': LinePlotModel,
    'FloatSpinBox': FloatSpinBoxModel,
    'DisplayStateColor': DisplayStateColorModel,
    'DisplayTableElement': TableElementModel,
    'EditableTableElement': TableElementModel,
    'Evaluator': EvaluatorModel,
    'TextIcons': TextIconsModel,
    'DigitIcons': DigitIconsModel,
    'SelectionIcons': SelectionIconsModel,
    'Monitor': MonitorModel,
    'SingleBit': SingleBitModel,
    'PopUp': PopUpModel,
    'StatefulIconWidget': StatefulIconWidgetModel,
}
