import base64
from xml.etree.ElementTree import SubElement

from traits.api import HasStrictTraits, Bool, Instance, List, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class DisplayIconsetModel(BaseWidgetObjectData):
    """ A model for DisplayIconset"""
    # A URL for an icon set (version 1 data only!)
    image = String
    # The actual icon set data
    data = Instance(bytes)


class IconData(HasStrictTraits):
    """ A base class for Icon (Item) data.
    """
    # XXX: Not sure what this is...
    equal = Bool
    # The value of the property
    value = String
    # A URL for an icon (version 1 data only!)
    image = String
    # The actual icon data
    # NOTE: This data will only be automatically loaded from version 2 files
    data = Instance(bytes)


class BaseIconsModel(BaseWidgetObjectData):
    """ A base class for Icons widgets.
    """
    # The icons shown here
    values = List(Instance(IconData))


class DigitIconsModel(BaseIconsModel):
    """ A model for DigitIcons"""


class SelectionIconsModel(BaseIconsModel):
    """ A model for SelectionIcons"""


class TextIconsModel(BaseIconsModel):
    """ A model for TextIcons"""


@register_scene_reader('DisplayIconset', version=2)
def _display_iconset_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['data'] = base64.b64decode(element.get('data'))
    return DisplayIconsetModel(**traits)


@register_scene_writer(DisplayIconsetModel)
def _display_iconset_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'DisplayIconset')
    element.set('data', base64.b64encode(model.data).decode("ascii"))
    return element


def _read_icon_elements(parent, tag):
    """ Read the icons for an icons widget.
    """
    icons = []
    for sub in parent:
        if sub.tag != tag:
            continue
        traits = {
            'value': sub.text or '',
            'data': base64.b64decode(sub.get('data')),
        }
        if sub.get('equal') is not None:
            traits['equal'] = True if sub.get('equal') == 'true' else False
        icons.append(IconData(**traits))
    return icons


def _write_icon_elements(icons, parent, tag):
    """ Write out the sub elements of an icons widget.
    """
    for ic in icons:
        sub = SubElement(parent, tag)
        uuencoded_data = base64.b64encode(ic.data).decode("ascii")
        sub.set('data', uuencoded_data)
        if ic.value:
            sub.text = ic.value
            if ic.equal:
                sub.set('equal', str(ic.equal).lower())


def _build_icon_widget_readers_and_writers():
    """ Build readers and writers for the BaseIconsModel classes
    """
    def _build_reader_func(klass, tag):
        def reader(read_func, element):
            traits = read_base_widget_data(element)
            traits['values'] = _read_icon_elements(element, NS_KARABO + tag)
            return klass(**traits)
        return reader

    def _build_writer_func(name, tag):
        def writer(write_func, model, parent):
            element = SubElement(parent, NS_SVG + 'rect')
            write_base_widget_data(model, element, name)
            _write_icon_elements(model.values, element, NS_KARABO + tag)
            return element
        return writer

    widgets = (('DigitIconsModel', 'value'), ('SelectionIconsModel', 'option'),
               ('TextIconsModel', 're'))
    for name, tag in widgets:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        reader = _build_reader_func(klass, tag)
        register_scene_reader(file_name, version=2)(reader)
        register_scene_writer(klass)(_build_writer_func(file_name, tag))

# Call the builder to register all the readers and writers
_build_icon_widget_readers_and_writers()


# -------------------------------------------------------------------------
# NOTE: Be very careful down here!
# Old reader code is maintained for the benefit of reading old files. It
# should only be modified in the most exceptional of situations!

@register_scene_reader('DisplayIconset', version=1)
def _display_iconset_version_1_reader(read_func, element):
    """ DisplayIconset reader for legacy scene files
    """
    traits = read_base_widget_data(element)
    image = element.get(NS_KARABO + 'url', '')
    if not image:
        # XXX: done to be compatible to older versions
        filename = element.get(NS_KARABO + 'filename')
        if filename is not None:
            image = filename
    traits['image'] = image
    return DisplayIconsetModel(**traits)


def _build_version_1_icon_widget_readers():
    """ Build BaseIconsModel readers for legacy scene files.
    """
    def _build_reader_func(klass, tag):
        def reader(read_func, element):
            traits = read_base_widget_data(element)
            icons = _read_icon_elements_version_1(element, NS_KARABO + tag)
            traits['values'] = icons
            return klass(**traits)
        return reader

    widgets = (('DigitIconsModel', 'value'), ('SelectionIconsModel', 'option'),
               ('TextIconsModel', 're'))
    for name, tag in widgets:
        klass = globals()[name]
        file_name = name[:-len('Model')]
        reader = _build_reader_func(klass, tag)
        register_scene_reader(file_name, version=1)(reader)


def _read_icon_elements_version_1(parent, tag):
    """ Read the icons for an icons widget.
    **NOTE**: This is a version 1 file reader. Do Not Modify!
    """
    icons = []
    for sub in parent:
        if sub.tag != tag:
            continue
        traits = {
            'image': sub.get('image', ''),
            'value': sub.text or ''
        }
        if sub.get('equal') is not None:
            traits['equal'] = True if sub.get('equal') == 'true' else False
        icons.append(IconData(**traits))
    return icons

# Call the builder to register the version 1 readers
_build_version_1_icon_widget_readers()
