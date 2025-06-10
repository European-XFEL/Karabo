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

from traits.api import Bool, CInt, Enum, Float, Int, String, Undefined

from karabo.common.scenemodel.bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseWidgetObjectData)
from karabo.common.scenemodel.const import (
    DEFAULT_DECIMALS, DEFAULT_FORMAT, NS_KARABO, SCENE_FONT_SIZE,
    SCENE_FONT_SIZES, SCENE_FONT_WEIGHT, SCENE_FONT_WEIGHTS,
    WIDGET_ELEMENT_TAG)
from karabo.common.scenemodel.io_utils import (
    get_numbers, read_alarm_data, read_base_widget_data,
    read_empty_display_editable_widget, read_font_format_data,
    read_value_format_data, set_numbers, write_alarm_data,
    write_base_widget_data, write_font_format_data, write_value_format_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class WidgetNodeModel(BaseWidgetObjectData):
    """A model for the basic widget node box"""


class CheckBoxModel(BaseDisplayEditableWidget):
    """A model for DisplayCheckBox/EditableCheckBox"""

    # The actual type of the widget
    klass = Enum("DisplayCheckBox", "EditableCheckBox")


class EditableChoiceElementModel(BaseEditWidget):
    """A model for DisplayChoiceElement/EditableChoiceElement"""


class EditableComboBoxModel(BaseEditWidget):
    """A model for EditableComboBox"""


class HistoricTextModel(BaseWidgetObjectData):
    """A model for the historic text element

    Note: This model is not associated with a reader and writer and cannot
    be written.
    """


class BaseLabelModel(BaseWidgetObjectData):
    """A model for DisplayLabel"""

    font_size = Enum(*SCENE_FONT_SIZES)
    font_weight = Enum(*SCENE_FONT_WEIGHTS)

    def _font_size_default(self):
        return SCENE_FONT_SIZE

    def _font_weight_default(self):
        return SCENE_FONT_WEIGHT


class DisplayLabelModel(BaseLabelModel):
    """A model for the DisplayLabel """


class DisplayListModel(BaseLabelModel):
    """A model for DisplayList"""


class DisplayFloatModel(BaseLabelModel):
    """A model for DisplayFloat"""
    fmt = String(DEFAULT_FORMAT)
    decimals = String(DEFAULT_DECIMALS)


class DisplayAlarmFloatModel(DisplayFloatModel):
    """A model for the Display Alarm Float Model"""
    alarmHigh = Float(Undefined)
    alarmLow = Float(Undefined)
    warnHigh = Float(Undefined)
    warnLow = Float(Undefined)


class DisplayAlarmIntegerModel(BaseLabelModel):
    """A model for the Display Alarm Float Model"""
    alarmHigh = CInt(Undefined)
    alarmLow = CInt(Undefined)
    warnHigh = CInt(Undefined)
    warnLow = CInt(Undefined)


class DisplayTextLogModel(BaseWidgetObjectData):
    """A model for DisplayTextLog"""


class EditableListModel(BaseEditWidget):
    """A model for EditableList"""


class EditableRegexListModel(BaseEditWidget):
    """A model for EditableRegexList"""


class EditableListElementModel(BaseEditWidget):
    """A model for EditableListElement"""


class EditableSpinBoxModel(BaseEditWidget):
    """A model for EditableSpinBox"""
    font_size = Enum(*SCENE_FONT_SIZES)
    font_weight = Enum(*SCENE_FONT_WEIGHTS)

    def __init__(self, font_size=SCENE_FONT_SIZE,
                 font_weight=SCENE_FONT_WEIGHT, **traits):
        # We set the default values as Enum doesn't set this straightforwardly
        super().__init__(font_size=font_size, font_weight=font_weight,
                         **traits)


class EditableRegexModel(BaseEditWidget):
    """A model for RegexEdit"""


class GlobalAlarmModel(BaseWidgetObjectData):
    """A model for GlobalAlarm Widget"""


class HexadecimalModel(BaseEditWidget):
    """A model for Hexadecimal"""


class IntLineEditModel(BaseEditWidget):
    """A model for IntLineEdit"""


class LabelModel(BaseWidgetObjectData):
    """A fragment of text which is shown in a scene."""

    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String("transparent")
    # The line width of a frame around the text
    frame_width = Int(0)
    # Horizontal alignment... default is align left (0x0001)
    alignh = Enum([1, 2, 4])


class LampModel(BaseWidgetObjectData):
    """A model for LampWidget"""


class LineEditModel(BaseDisplayEditableWidget):
    """A model for DisplayLineEdit/EditableLineEdit"""

    # The actual type of the widget
    klass = Enum("DisplayLineEdit", "EditableLineEdit")


class StickerModel(BaseWidgetObjectData):
    """A model for a Sticker widget"""

    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String("white")


class TickSliderModel(BaseEditWidget):
    """A model for TickSlider"""
    ticks = Int(1)
    show_value = Bool(True)


class DisplayTimeModel(BaseLabelModel):
    """A model for the time widget"""
    time_format = String("%H:%M:%S")


class InstanceStatusModel(BaseWidgetObjectData):
    """A model to display the topology status of a device on the scene"""


class PopupButtonModel(BaseWidgetObjectData):
    """A mode for a popup button widget"""
    # The text to be displayed in popup
    text = String
    # The text to displayed on the button
    label = String

    # The width of the tooltip in pixels
    popup_width = CInt
    popup_height = CInt


def _read_geometry_data(element):
    """Read the geometry information."""
    return get_numbers(("x", "y", "width", "height"), element)


def _write_class_and_geometry(model, element, widget_class_name):
    """Write out the class name and geometry information."""
    element.set(NS_KARABO + "class", widget_class_name)
    set_numbers(("x", "y", "width", "height"), model, element)


@register_scene_reader("Label", version=1)
def __label_reader(element):
    """A reader for Label objects in Version 1 scenes"""
    traits = _read_geometry_data(element)
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font", "")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")
    traits["alignh"] = int(element.get(NS_KARABO + "alignh", 1))

    bg = element.get(NS_KARABO + "background")
    if bg is not None:
        traits["background"] = bg
    fw = element.get(NS_KARABO + "frameWidth")
    if fw is not None:
        traits["frame_width"] = int(fw)
    return LabelModel(**traits)


@register_scene_writer(LabelModel)
def __label_writer(model, parent):
    """A writer for LabelModel objects"""
    element = SubElement(parent, WIDGET_ELEMENT_TAG)

    _write_class_and_geometry(model, element, "Label")

    for name in ("text", "font", "foreground"):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + "frameWidth", str(model.frame_width))
    if model.background != "":
        element.set(NS_KARABO + "background", model.background)
    if model.alignh != 1:
        element.set(NS_KARABO + "alignh", str(model.alignh))

    return element


@register_scene_reader("TickSlider")
def _tick_slider_reader(element):
    traits = read_base_widget_data(element)
    traits["ticks"] = int(element.get(NS_KARABO + "ticks", 1))
    show_value = element.get(NS_KARABO + "show_value", "true")
    traits["show_value"] = show_value.lower() == "true"

    return TickSliderModel(**traits)


@register_scene_writer(TickSliderModel)
def _tick_slider_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "TickSlider")
    element.set(NS_KARABO + "ticks", str(model.ticks))
    element.set(NS_KARABO + "show_value", str(model.show_value).lower())

    return element


@register_scene_reader("TimeLabel")
def _time_label_reader(element):
    traits = read_base_widget_data(element)
    time_format = element.get(NS_KARABO + "time_format", "%H:%M:%S")
    traits["time_format"] = time_format
    traits.update(read_font_format_data(element))
    return DisplayTimeModel(**traits)


@register_scene_writer(DisplayTimeModel)
def _time_label_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "TimeLabel")
    element.set(NS_KARABO + "time_format", str(model.time_format))
    write_font_format_data(model, element)

    return element


@register_scene_reader("DisplayLabel")
def _display_label_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))

    return DisplayLabelModel(**traits)


@register_scene_writer(DisplayLabelModel)
def _display_label_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayLabel")
    write_font_format_data(model, element)

    return element


@register_scene_reader("DisplayFloat")
def _display_float_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    traits.update(read_value_format_data(element))

    return DisplayFloatModel(**traits)


@register_scene_writer(DisplayFloatModel)
def _display_float_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayFloat")
    write_font_format_data(model, element)
    write_value_format_data(model, element)

    return element


@register_scene_reader("DisplayAlarmFloat")
def _display_alarm_float_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    traits.update(read_value_format_data(element))
    traits.update(read_alarm_data(element))

    return DisplayAlarmFloatModel(**traits)


@register_scene_writer(DisplayAlarmFloatModel)
def _display_alarm_float_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayAlarmFloat")
    write_font_format_data(model, element)
    write_value_format_data(model, element)
    write_alarm_data(model, element)

    return element


@register_scene_reader("DisplayAlarmInteger")
def _display_alarm_integer_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    traits.update(read_alarm_data(element))

    return DisplayAlarmIntegerModel(**traits)


@register_scene_writer(DisplayAlarmIntegerModel)
def _display_alarm_integer_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayAlarmInteger")
    write_font_format_data(model, element)
    write_alarm_data(model, element)

    return element


@register_scene_reader("StickerWidget")
def __sticker_widget_reader(element):
    traits = _read_geometry_data(element)
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["font"] = element.get(NS_KARABO + "font", "")
    traits["foreground"] = element.get(NS_KARABO + "foreground", "black")

    bg = element.get(NS_KARABO + "background")
    if bg is not None:
        traits["background"] = bg

    return StickerModel(**traits)


@register_scene_writer(StickerModel)
def __sticker_widget_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, "StickerWidget")
    for name in ("text", "font", "foreground"):
        element.set(NS_KARABO + name, getattr(model, name))

    if model.background != "":
        element.set(NS_KARABO + "background", model.background)

    return element


@register_scene_reader("PopupButtonWidget")
def __popup_sticker_widget_reader(element):
    traits = _read_geometry_data(element)
    traits["text"] = element.get(NS_KARABO + "text", "")
    traits["label"] = element.get(NS_KARABO + "label", "")
    traits["popup_width"] = int(element.get("popup_width", 200))
    traits["popup_height"] = int(element.get("popup_height", 100))
    return PopupButtonModel(**traits)


@register_scene_writer(PopupButtonModel)
def __popup_sticker_widget_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    _write_class_and_geometry(model, element, "PopupButtonWidget")
    for name in ("text", "label"):
        element.set(NS_KARABO + name, getattr(model, name))
    set_numbers(("popup_width", "popup_height"), model, element)
    return element


@register_scene_reader("EditableSpinBox")
def _editable_spinbox_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    return EditableSpinBoxModel(**traits)


@register_scene_writer(EditableSpinBoxModel)
def _editable_spinbox_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "EditableSpinBox")
    write_font_format_data(model, element)

    return element


@register_scene_reader("DisplayList")
def _display_list_reader(element):
    traits = read_base_widget_data(element)
    traits.update(read_font_format_data(element))
    return DisplayListModel(**traits)


@register_scene_reader("EditableList")
def _editable_list_reader(element):
    traits = read_base_widget_data(element)
    if traits["parent_component"] == "DisplayComponent":
        # We have been writing this model wrong for around X years.
        # The correct model is the `DisplayListModel` ...
        return DisplayListModel(**traits)
    return EditableListModel(**traits)


@register_scene_writer(DisplayListModel)
def _display_list_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplayList")
    write_font_format_data(model, element)
    return element


@register_scene_writer(EditableListModel)
def _editable_list_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "EditableList")


def _build_empty_widget_readers_and_writers():
    """Build readers and writers for the empty widget classes"""

    def _build_reader_func(klass):
        def reader(element):
            traits = read_base_widget_data(element)
            return klass(**traits)

        return reader

    def _build_writer_func(name):
        def writer(model, parent):
            element = SubElement(parent, WIDGET_ELEMENT_TAG)
            write_base_widget_data(model, element, name)
            return element

        return writer

    names = (
        "DisplayTextLogModel",
        "EditableComboBoxModel",
        "EditableRegexListModel",
        "EditableListElementModel",
        "EditableRegexModel",
        "EditableChoiceElementModel",
        "GlobalAlarmModel",
        "HexadecimalModel",
        "HistoricTextModel",
        "IntLineEditModel",
        "LampModel",
        "WidgetNodeModel",
        "InstanceStatusModel",
    )
    for name in names:
        klass = globals()[name]
        file_name = name[: -len("Model")]
        register_scene_reader(file_name, version=1)(_build_reader_func(klass))
        register_scene_writer(klass)(_build_writer_func(file_name))


def _build_empty_display_editable_readers_and_writers():
    """Build readers and writers for the empty widget classes which come in
    Editable and Display types.
    """

    def _build_reader_func(klass):
        def reader(element):
            traits = read_empty_display_editable_widget(element)
            return klass(**traits)

        return reader

    def _writer_func(model, parent):
        element = SubElement(parent, WIDGET_ELEMENT_TAG)
        write_base_widget_data(model, element, model.klass)
        return element

    names = (
        "CheckBoxModel",
        "LineEditModel",
    )
    for name in names:
        klass = globals()[name]
        file_name = name[: -len("Model")]
        reader = _build_reader_func(klass)
        register_scene_reader("Display" + file_name, version=1)(reader)
        register_scene_reader("Editable" + file_name, version=1)(reader)
        register_scene_writer(klass)(_writer_func)


# Call the builders to register all the readers and writers
_build_empty_widget_readers_and_writers()
_build_empty_display_editable_readers_and_writers()
