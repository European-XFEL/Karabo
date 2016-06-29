#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QBoxLayout

from karabo_gui.scenemodel.api import (
    BitfieldModel, CheckBoxModel, ChoiceElementModel, ComboBoxModel,
    DirectoryModel, DisplayAlignedImageModel, DisplayCommandModel,
    DisplayIconsetModel, DisplayImageModel, DisplayImageElementModel,
    DisplayLabelModel, DisplayPlotModel, DisplayStateColorModel,
    DoubleLineEditModel, EditableListModel, EditableListElementModel,
    EditableSpinBoxModel, EvaluatorModel, FileInModel, FileOutModel,
    FloatSpinBoxModel, HexadecimalModel, DigitIconsModel,
    SelectionIconsModel, TextIconsModel, IntLineEditModel, KnobModel,
    LineEditModel, LinePlotModel, MonitorModel, SingleBitModel,
    SliderModel, TableElementModel, XYPlotModel
)

# A handy limit for things measured in pixels
SCREEN_MAX_VALUE = 100000

# For convenience association of strings to Qt cursors
QT_CURSORS = {
    'arrow': Qt.ArrowCursor,
    'blank': Qt.BlankCursor,
    'busy': Qt.BusyCursor,
    'cross': Qt.CrossCursor,
    'closed-hand': Qt.ClosedHandCursor,
    'open-hand': Qt.OpenHandCursor,
    'pointing-hand': Qt.PointingHandCursor,
    'resize-diagonal-trbl': Qt.SizeBDiagCursor,
    'resize-diagonal-tlbr': Qt.SizeFDiagCursor,
    'resize-horizontal': Qt.SizeHorCursor,
    'resize-vertical': Qt.SizeVerCursor,
}

# For convenience association of strings to Qt enums
QT_PEN_CAP_STYLE_FROM_STR = {
    'butt': Qt.FlatCap,
    'square': Qt.SquareCap,
    'round': Qt.RoundCap,
}

# For convenience association reverse mapping of Qt enums to string
QT_PEN_CAP_STYLE_TO_STR = {v: k for k, v in QT_PEN_CAP_STYLE_FROM_STR.items()}

# For convenience association of strings to Qt enums
QT_PEN_JOIN_STYLE_FROM_STR = {
    'miter': Qt.MiterJoin,
    'svgmiter': Qt.SvgMiterJoin,
    'round': Qt.RoundJoin,
    'bevel': Qt.BevelJoin,
}

# For convenience association reverse mapping of Qt enums to string
QT_PEN_JOIN_STYLE_TO_STR = {v: k
                            for k, v in QT_PEN_JOIN_STYLE_FROM_STR.items()}

# For convenience association of ints to Qt enums
QT_BOX_LAYOUT_DIRECTION = (
    QBoxLayout.LeftToRight,
    QBoxLayout.RightToLeft,
    QBoxLayout.TopToBottom,
    QBoxLayout.BottomToTop
)

_OLD_WIDGET_CLASSES = {
    'DisplayAlignedImage': DisplayAlignedImageModel,
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
    'DisplayImage': DisplayImageModel,
    'DisplayImageElement': DisplayImageElementModel,
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
}
