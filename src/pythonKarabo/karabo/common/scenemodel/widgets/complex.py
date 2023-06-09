# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from xml.etree.ElementTree import SubElement

from traits.api import Bool, Enum, Float, Int, String

from karabo.common.scenemodel.bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseWidgetObjectData)
from karabo.common.scenemodel.const import (
    NS_KARABO, SCENE_FONT_SIZE, SCENE_FONT_SIZES, SCENE_FONT_WEIGHT,
    SCENE_FONT_WEIGHTS, WIDGET_ELEMENT_TAG)
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, read_empty_display_editable_widget,
    read_font_format_data, write_base_widget_data, write_font_format_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)

from .simple import DisplayLabelModel


class ColorBoolModel(BaseWidgetObjectData):
    """A model for DisplayBool Widget"""

    invert = Bool(False)


class DisplayCommandModel(BaseWidgetObjectData):
    """A model for DisplayCommand"""
    requires_confirmation = Bool(False)
    font_size = Enum(*SCENE_FONT_SIZES)
    font_weight = Enum(*SCENE_FONT_WEIGHTS)

    def __init__(self, font_size=SCENE_FONT_SIZE,
                 font_weight=SCENE_FONT_WEIGHT, **traits):
        super().__init__(font_size=font_size, font_weight=font_weight,
                         **traits)


class DisplayIconCommandModel(BaseWidgetObjectData):
    """A model for DisplayIconCommand"""

    icon_name = String


class DoubleLineEditModel(BaseEditWidget):
    """A model for DoubleLineEdit Widget"""

    # The floating point precision
    decimals = Int(-1)


class DisplayProgressBarModel(BaseWidgetObjectData):
    """A model for Progress Bar"""

    # the orientation of the progress bar
    is_vertical = Bool(False)


class DisplayStateColorModel(DisplayLabelModel):
    """A model for DisplayStateColor"""

    # The state shown on the widget
    show_string = Bool(False)


class ErrorBoolModel(BaseWidgetObjectData):
    """A model for ErrorBool Widget"""

    invert = Bool(False)


class EvaluatorModel(DisplayLabelModel):
    """A model for Evaluator"""

    # The expression which is evaluated
    expression = String("x")


class FloatSpinBoxModel(BaseEditWidget):
    """A model for FloatSpinBox"""

    # The step size
    step = Float
    decimals = Int(3)
    font_size = Enum(*SCENE_FONT_SIZES)
    font_weight = Enum(*SCENE_FONT_WEIGHTS)

    def __init__(self, font_size=SCENE_FONT_SIZE,
                 font_weight=SCENE_FONT_WEIGHT, **traits):
        # We set the default values as Enum doesn't set this straightforwardly
        super().__init__(font_size=font_size, font_weight=font_weight,
                         **traits)


class MonitorModel(BaseWidgetObjectData):
    """A model for Monitor"""

    # A file path
    filename = String
    # An interval
    interval = Float


class SingleBitModel(BaseWidgetObjectData):
    """A model for SingleBit"""

    # Which bit is being displayed
    bit = Int
    # True if the value of the bit should be inverted
    invert = Bool(False)


class TableElementModel(BaseDisplayEditableWidget):
    """A model for TableElement"""

    # The actual type of the widget
    klass = Enum("DisplayTableElement", "EditableTableElement")
    # True if the table is resizing the columns to contents
    resizeToContents = Bool(False)


class FilterTableElementModel(BaseDisplayEditableWidget):
    """A model for FilterTableElement"""

    klass = Enum("DisplayFilterTableElement", "EditableFilterTableElement")
    # True if the table is resizing the columns to contents
    resizeToContents = Bool(False)
    sortingEnabled = Bool(False)
    filterKeyColumn = Int(0)
    showFilterKeyColumn = Bool(False)


@register_scene_reader("DisplayCommand", version=2)
def __display_command_reader(element):
    confirmation = element.get(NS_KARABO + "requires_confirmation", "")
    confirmation = confirmation.lower() == "true"
    traits = read_base_widget_data(element)
    traits["requires_confirmation"] = confirmation
    traits.update(read_font_format_data(element))

    return DisplayCommandModel(**traits)


@register_scene_writer(DisplayCommandModel)
def __display_command_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayCommand")
    element.set(
        NS_KARABO + "requires_confirmation",
        str(model.requires_confirmation).lower(),
    )
    write_font_format_data(model, element)
    return element


@register_scene_reader("DisplayIconCommand")
def __display_icon_command_reader(element):
    traits = read_base_widget_data(element)
    traits["icon_name"] = element.get(NS_KARABO + "icon_name", "")

    return DisplayIconCommandModel(**traits)


@register_scene_writer(DisplayIconCommandModel)
def __display_icon_command_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayIconCommand")
    element.set(NS_KARABO + "icon_name", model.icon_name)
    return element


@register_scene_reader("DisplayColorBool", version=2)
def _color_bool_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + "invert")
    traits["invert"] = value.lower() == "true"
    return ColorBoolModel(**traits)


@register_scene_writer(ColorBoolModel)
def _color_bool_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayColorBool")
    element.set(NS_KARABO + "invert", str(model.invert).lower())
    return element


@register_scene_reader("DisplayErrorBool")
def _error_bool_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + "invert")
    traits["invert"] = value.lower() == "true"
    return ErrorBoolModel(**traits)


@register_scene_writer(ErrorBoolModel)
def _error_bool_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayErrorBool")
    element.set(NS_KARABO + "invert", str(model.invert).lower())
    return element


@register_scene_reader("DoubleLineEdit", version=1)
def _double_line_edit_reader(element):
    traits = read_base_widget_data(element)
    decimals = element.get(NS_KARABO + "decimals", "")
    if decimals:
        traits["decimals"] = int(decimals)
    return DoubleLineEditModel(**traits)


@register_scene_writer(DoubleLineEditModel)
def _double_line_edit_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DoubleLineEdit")
    element.set(NS_KARABO + "decimals", str(model.decimals))
    return element


@register_scene_reader("DisplayProgressBar", version=2)
def _display_progress_bar_reader(element):
    traits = read_base_widget_data(element)
    value = element.get(NS_KARABO + "is_vertical", "")
    traits["is_vertical"] = value.lower() == "true"
    return DisplayProgressBarModel(**traits)


@register_scene_writer(DisplayProgressBarModel)
def _display_progress_bar_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayProgressBar")
    element.set(NS_KARABO + "is_vertical", str(model.is_vertical).lower())
    return element


@register_scene_reader("DisplayStateColor", version=1)
def _display_state_color_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    value = element.get(NS_KARABO + "show_string", "")
    traits["show_string"] = value.lower() == "true"
    return DisplayStateColorModel(**traits)


@register_scene_writer(DisplayStateColorModel)
def _display_state_color_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayStateColor")
    write_font_format_data(model, element)
    element.set(NS_KARABO + "show_string", str(model.show_string).lower())
    return element


@register_scene_reader("Evaluator", version=1)
def _evaluator_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    traits["expression"] = element.get("expression")
    return EvaluatorModel(**traits)


@register_scene_writer(EvaluatorModel)
def _evaluator_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "Evaluator")
    write_font_format_data(model, element)
    element.set("expression", model.expression)
    return element


@register_scene_reader("FloatSpinBox", version=1)
def _float_spin_box_reader(element):
    traits = read_base_widget_data(element)
    traits["step"] = float(element.get(NS_KARABO + "step", 1.0))
    decimals = element.get(NS_KARABO + "decimals", "")
    if decimals:
        traits["decimals"] = int(decimals)
    traits.update(read_font_format_data(element))
    return FloatSpinBoxModel(**traits)


@register_scene_writer(FloatSpinBoxModel)
def _float_spin_box_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "FloatSpinBox")
    element.set(NS_KARABO + "step", str(model.step))
    element.set(NS_KARABO + "decimals", str(model.decimals))
    write_font_format_data(model, element)

    return element


@register_scene_reader("Monitor", version=1)
def _monitor_reader(element):
    traits = read_base_widget_data(element)
    traits["filename"] = element.get("filename", "")
    traits["interval"] = float(element.get("interval"))
    return MonitorModel(**traits)


@register_scene_writer(MonitorModel)
def _monitor_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "Monitor")
    element.set("interval", str(model.interval))
    if len(model.filename) > 0:
        element.set("filename", model.filename)
    return element


@register_scene_reader("SingleBit", version=1)
def _single_bit_reader(element):
    traits = read_base_widget_data(element)
    invert = element.get(NS_KARABO + "invert", "")
    traits["invert"] = invert.lower() == "true"
    traits["bit"] = int(element.get(NS_KARABO + "bit", 0))
    return SingleBitModel(**traits)


@register_scene_writer(SingleBitModel)
def _single_bit_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "SingleBit")
    element.set(NS_KARABO + "bit", str(model.bit))
    element.set(NS_KARABO + "invert", str(model.invert).lower())
    return element


@register_scene_reader("DisplayTableElement", version=1)
@register_scene_reader("EditableTableElement", version=1)
def _table_element_reader(element):
    traits = read_empty_display_editable_widget(element)
    resizeToContents = element.get(NS_KARABO + "resizeToContents", "")
    resizeToContents = resizeToContents.lower() == "true"
    traits["resizeToContents"] = resizeToContents
    return TableElementModel(**traits)


@register_scene_writer(TableElementModel)
def _table_element_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    element.set(
        NS_KARABO + "resizeToContents", str(model.resizeToContents).lower()
    )
    return element


@register_scene_reader("DisplayFilterTableElement")
@register_scene_reader("EditableFilterTableElement")
def _filter_table_element_reader(element):
    traits = read_empty_display_editable_widget(element)
    resizeToContents = element.get(NS_KARABO + "resizeToContents", "")
    resizeToContents = resizeToContents.lower() == "true"
    traits["resizeToContents"] = resizeToContents
    sortingEnabled = element.get(NS_KARABO + "sortingEnabled", "")
    sortingEnabled = sortingEnabled.lower() == "true"
    traits["sortingEnabled"] = sortingEnabled
    filterKeyColumn = int(element.get(NS_KARABO + "filterKeyColumn", 0))
    traits["filterKeyColumn"] = filterKeyColumn
    showFilterKeyColumn = element.get(NS_KARABO + "showFilterKeyColumn", "")
    showFilterKeyColumn = showFilterKeyColumn.lower() == "true"
    traits["showFilterKeyColumn"] = showFilterKeyColumn

    return FilterTableElementModel(**traits)


@register_scene_writer(FilterTableElementModel)
def _filter_table_element_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    element.set(NS_KARABO + "resizeToContents",
                str(model.resizeToContents).lower())
    element.set(NS_KARABO + "sortingEnabled",
                str(model.sortingEnabled).lower())
    element.set(NS_KARABO + "filterKeyColumn", str(model.filterKeyColumn))
    element.set(NS_KARABO + "showFilterKeyColumn",
                str(model.showFilterKeyColumn))

    return element
