#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 30, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtGui import QBoxLayout, QFont

from traits.api import ABCHasStrictTraits

from karabo_gui.enums import NavigationItemTypes
from karabo_gui.schema import ChoiceOfNodes
from karabo_gui.topology import getDeviceBox
from karabo_gui.widget import DisplayWidget, EditableWidget
from karabo_gui.sceneview.utils import calc_rect_from_text

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
    SliderModel, TableElementModel, XYPlotModel, BoxLayoutModel, LabelModel
)

_WIDGET_FACTORIES = {
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


class SceneDnDHandler(ABCHasStrictTraits):

    @abstractmethod
    def can_handle(self, event):
        """ Check whether the drag event can be handled. """

    @abstractmethod
    def handle(self, scene_view, event):
        """ Handle the drop event. """


class ConfigurationDropHandler(SceneDnDHandler):

    def can_handle(self, event):
        sourceType = event.mimeData().data("sourceType")
        if sourceType == "ParameterTreeWidget":
            source = event.source()
            if (source is not None and not (source.conf.type == "class")
                    and not isinstance(source.currentItem().box.descriptor,
                                       ChoiceOfNodes)):
                return True
        return False

    def handle(self, scene_view, event):
        mimeData = event.mimeData()
        sourceType = mimeData.data("sourceType")
        source = event.source()

        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None:
            boxes = [item.box for item in source.selectedItems()]
            if widget.add_boxes(boxes):
                return

        if sourceType == "ParameterTreeWidget":
            selectedItems = source.selectedItems()
            for item in selectedItems:
                layout_model = self._create_model_from_parameter_item(item,
                                                                      pos)
                scene_view.add_models(layout_model)
        event.accept()

    def _create_model_from_parameter_item(self, item, pos):
        """ The given ``item`` which is a TreeWidgetItem is used to create
            the model for the view."""
        # Horizonal layout
        layout_model = BoxLayoutModel(direction=QBoxLayout.LeftToRight)
        layout_model.x = pos.x()
        layout_model.y = pos.y()
        label_model = LabelModel(text=item.text(0))
        label_model.font = QFont().toString()
        label_model.foreground = '#000000'

        # Calculate geometry for label
        x, y, width, height = calc_rect_from_text(label_model.font,
                                                  label_model.text)
        label_model.x = x
        label_model.y = y
        label_model.width = width
        label_model.height = height
        # Update geometry of layout model
        layout_model.width += label_model.width
        layout_model.height = max(layout_model.height, label_model.height)
        # Add label to layout model
        layout_model.children.append(label_model)

        # Get Boxes. "box" is in the project, "realbox" the
        # one on the device. They are the same if not from a project
        box = item.box
        realbox = getDeviceBox(box)
        if realbox.descriptor is not None:
            box = realbox

        MODEL_WIDTH = 150
        MODEL_HEIGHT = 43

        display_component = item.displayComponent
        if display_component is not None:
            factory = DisplayWidget.getClass(box)
            traits = {'x': 0, 'y': 0, 'width': MODEL_WIDTH,
                      'height': MODEL_HEIGHT, 'keys': [box.key()],
                      'parent_component': 'DisplayComponent'}

            klass = _WIDGET_FACTORIES[factory.__name__]
            self._add_model_to_layout(klass, traits, layout_model)

        edit_component = item.editableComponent
        if edit_component is not None:
            factory = EditableWidget.getClass(box)
            traits = {'x': 0, 'y': 0, 'width': MODEL_WIDTH + 50,
                      'height': MODEL_HEIGHT, 'keys': [box.key()],
                      'parent_component': 'EditableApplyLaterComponent'}

            klass = _WIDGET_FACTORIES[factory.__name__]
            self._add_model_to_layout(klass, traits, layout_model)

        return layout_model

    def _add_model_to_layout(self, klass, traits, layout_model):
        """ """
        model = klass(**traits)
        layout_model.width += model.width
        layout_model.height = max(layout_model.height, model.height)
        # Add label to layout model
        layout_model.children.append(model)


class NavigationDropHandler(SceneDnDHandler):

    def can_handle(self, event):
        """ Check whether the drag event can be handled. """
        sourceType = event.mimeData().data("sourceType")
        if sourceType == "NavigationTreeView":
            source = event.source()
            type = source.indexInfo().get("type")
            if source is not None and type == NavigationItemTypes.CLASS:
                return True
        return False

    def handle(self, scene_view, event):
        """ Handle the drop event. """
