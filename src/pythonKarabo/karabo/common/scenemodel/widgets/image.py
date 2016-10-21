#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import SubElement

from traits.api import Bool

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (read_base_widget_data,
                                               write_base_widget_data)
from karabo.common.scenemodel.registry import (register_scene_reader,
                                               register_scene_writer)


class DisplayAlignedImageModel(BaseWidgetObjectData):
    """ A model for DisplayAlignedImage"""


class DisplayImageModel(BaseWidgetObjectData):
    """ A model for DisplayImage"""


class DisplayImageElementModel(BaseWidgetObjectData):
    """ A model for DisplayImageElement"""


class BaseImageModel(BaseWidgetObjectData):
    show_tool_bar = Bool(True)
    show_color_bar = Bool(True)
    show_axes = Bool(True)


class ScientificImageModel(BaseImageModel):
    """ A model for ScientificImageDisplay"""


class WebcamImageModel(BaseImageModel):
    """ A model for WebcamImageDisplay"""


def _build_simple_image_widget_readers_and_writers():
    """ Build readers and writers for the empty image widget classes
    """
    def _build_reader_func(klass):
        def reader(read_func, element):
            traits = read_base_widget_data(element)
            return klass(**traits)
        return reader

    def _build_writer_func(name):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            write_base_widget_data(model, element, name)
            return element
        return writer

    names = ('DisplayAlignedImageModel', 'DisplayImageModel',
             'DisplayImageElementModel')
    for name in names:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        register_scene_reader(file_name, version=1)(_build_reader_func(klass))
        register_scene_writer(klass)(_build_writer_func(file_name))


def _build_complex_image_widget_readers_and_writers():
    """ Build readers and writers for the empty image widget classes
    """
    def _build_reader_func(klass):
        def reader(read_func, element):
            traits = read_base_widget_data(element)
            bool_names = ('show_tool_bar', 'show_color_bar', 'show_axes')
            for b_name in bool_names:
                traits[b_name] = (element.get(NS_KARABO + b_name) == 'true')
            return klass(**traits)
        return reader

    def _build_writer_func(name):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            write_base_widget_data(model, element, name)
            bool_names = ('show_tool_bar', 'show_color_bar', 'show_axes')
            for b_name in bool_names:
                element.set(
                    NS_KARABO + b_name, str(getattr(model, b_name)).lower())
            return element
        return writer

    names = ('WebcamImageModel', 'ScientificImageModel')
    for name in names:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        register_scene_reader(file_name, version=1)(_build_reader_func(klass))
        register_scene_writer(klass)(_build_writer_func(file_name))


# Call the builders to register all the readers and writers
_build_simple_image_widget_readers_and_writers()
_build_complex_image_widget_readers_and_writers()
