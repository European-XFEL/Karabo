from karabo.common.scenemodel import api as models


# This is a mapping of Widget class names -> scene model classes
# It is used in situations where ``Widget`` is asked for a list of classes
# which can work together with a ``Box`` object. The class name is used to
# fetch a scene model which can be added to the scene which will result in a
# widget which works with a box.
# Examples:
# * Switching widgets for a device property
# * Adding a default widget when a property is dropped on the scene
WIDGET_FACTORIES = {
    'Bitfield': models.BitfieldModel,
    'DigitIcons': models.DigitIconsModel,
    'DisplayAlignedImage': models.DisplayAlignedImageModel,
    'DisplayAnalog': models.AnalogModel,
    'DisplayCheckBox': models.CheckBoxModel,
    'DisplayChoiceElement': models.ChoiceElementModel,
    'DisplayColorBool': models.ColorBoolModel,
    'DisplayComboBox': models.ComboBoxModel,
    'DisplayCommand': models.DisplayCommandModel,
    'DisplayDirectory': models.DirectoryModel,
    'DisplayFileIn': models.FileInModel,
    'DisplayFileOut': models.FileOutModel,
    'DisplayIconset': models.DisplayIconsetModel,
    'DisplayImage': models.DisplayImageModel,
    'DisplayImageElement': models.DisplayImageElementModel,
    'DisplayLabel': models.DisplayLabelModel,
    'DisplayLineEdit': models.LineEditModel,
    'DisplayPlot': models.DisplayPlotModel,
    'DisplayProgressBar': models.DisplayProgressBarModel,
    'DisplaySparkline': models.SparklineModel,
    'DisplayStateColor': models.DisplayStateColorModel,
    'DisplayTableElement': models.TableElementModel,
    'DisplayTextLog': models.DisplayTextLogModel,
    'DisplayTrendline': models.LinePlotModel,
    'DoubleLineEdit': models.DoubleLineEditModel,
    'EditableCheckBox': models.CheckBoxModel,
    'EditableChoiceElement': models.ChoiceElementModel,
    'EditableComboBox': models.ComboBoxModel,
    'EditableDirectory': models.DirectoryModel,
    'EditableFileIn': models.FileInModel,
    'EditableFileOut': models.FileOutModel,
    'EditableLineEdit': models.LineEditModel,
    'EditableList': models.EditableListModel,
    'EditableListElement': models.EditableListElementModel,
    'EditableSpinBox': models.EditableSpinBoxModel,
    'EditableTableElement': models.TableElementModel,
    'Evaluator': models.EvaluatorModel,
    'FloatSpinBox': models.FloatSpinBoxModel,
    'Hexadecimal': models.HexadecimalModel,
    'IntLineEdit': models.IntLineEditModel,
    'Knob': models.KnobModel,
    'LampWidget': models.LampModel,
    'MultiCurvePlot': models.MultiCurvePlotModel,
    'Monitor': models.MonitorModel,
    'PopUp': models.PopUpModel,
    'RunConfiguratorEdit': models.RunConfiguratorModel,
    'ScientificImageDisplay': models.ScientificImageModel,
    'SelectionIcons': models.SelectionIconsModel,
    'SingleBit': models.SingleBitModel,
    'Slider': models.SliderModel,
    'StatefulIconWidget': models.StatefulIconWidgetModel,
    'TextIcons': models.TextIconsModel,
    'WebcamImageDisplay': models.WebcamImageModel,
    'XYPlot': models.XYPlotModel,
    'XYVector': models.LinePlotModel,
}
