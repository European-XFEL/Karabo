from xml.etree.ElementTree import SubElement

from traits.api import Float, String

from .bases import BaseShapeObjectData
from .const import NS_SVG
from .io_utils import get_numbers, set_numbers
from .registry import register_scene_reader, register_scene_writer


class LineModel(BaseShapeObjectData):
    """ A line which can appear in a scene
    """
    # The X-coordinate of the first point
    x1 = Float
    # The Y-coordinate of the first point
    y1 = Float
    # The X-coordinate of the second point
    x2 = Float
    # The Y-coordinate of the second point
    y2 = Float


class PathModel(BaseShapeObjectData):
    """ An arbitrary path object for scenes
    """
    # A blob of SVG data...
    svg_data = String


class RectangleModel(BaseShapeObjectData):
    """ A rectangle which can appear in a scene
    """
    # The X-coordinate of the rect
    x = Float
    # The Y-coordinate of the rect
    y = Float
    # The height of the rect
    height = Float
    # The width of the rect
    width = Float


def _convert_measurement(measure):
    """ Convert a measurement value to pixels
    """
    scales = {
        "px": 1, "pt": 1.25, "pc": 15, "mm": 3.543307, "cm": 35.43307, "in": 90
    }
    scale = scales.get(measure[-2:])
    if scale is not None:
        return scale * float(measure[:-2])
    else:
        return float(measure)


def _read_base_shape_data(element):
    """ Read the style attributes common to all "shape" elements
    """
    # Break up a style attribute if that's where the style info is at.
    d = element.attrib.copy()
    if 'style' in d:
        d.update(s.split(':') for s in d['style'].split(';'))

    # Read all the trait values
    converters = {
        'stroke': str, 'stroke-opacity': float, 'stroke-linecap': str,
        'stroke-dashoffset': _convert_measurement,
        'stroke-width': _convert_measurement, 'stroke-dasharray': str,
        'stroke-style': int, 'stroke-linejoin': str,
        'stroke-miterlimit': float, 'fill': str, 'fill-opacity': float,
    }
    traits = {name.replace('-', '_'): converters[name](d[name])
              for name in converters if name in d}

    # Replicate behavior of old (pre-1.5) scene view
    if 'fill' not in traits:
        traits['fill'] = 'black'

    # Convert the dash array to a proper value
    if 'stroke_dasharray' in traits:
        dashes = traits.pop('stroke_dasharray')
        if dashes.lower() != 'none':
            dashlist = dashes.split(',') if ',' in dashes else dashes.split()
            penwidth = traits.get('stroke_width', 1.0)
            traits['stroke_dasharray'] = [_convert_measurement(d) / penwidth
                                          for d in dashlist]

    return traits


def _write_base_shape_data(model, element):
    """ Write out the style attributes common to all "shape" elements
    """
    write = element.set

    write('stroke', model.stroke)
    if model.stroke != 'none':
        write('stroke-opacity', str(model.stroke_opacity))
        write('stroke-linecap', model.stroke_linecap)
        write('stroke-dashoffset', str(model.stroke_dashoffset))
        write('stroke-width', str(model.stroke_width))
        write('stroke-dasharray', " ".join(str(x * model.stroke_width)
                                           for x in model.stroke_dasharray))
        write('stroke-style', str(model.stroke_style))
        write('stroke-linejoin', model.stroke_linejoin)
        write('stroke-miterlimit', str(model.stroke_miterlimit))

    write('fill', model.fill)
    if model.fill != 'none':
        write('fill-opacity', str(model.fill_opacity))


@register_scene_reader('Line', xmltag=NS_SVG + 'line', version=1)
def __line_reader(element):
    """ A reader for Line objects in Version 1 scenes
    """
    traits = _read_base_shape_data(element)
    traits.update(get_numbers(('x1', 'y1', 'x2', 'y2'), element))
    return LineModel(**traits)


@register_scene_writer(LineModel)
def __line_writer(write_func, model, parent):
    """ A writer for LineModel objects
    """
    element = SubElement(parent, NS_SVG + 'line')
    _write_base_shape_data(model, element)
    set_numbers(('x1', 'y1', 'x2', 'y2'), model, element)
    return element


@register_scene_reader('Path', xmltag=NS_SVG + 'path', version=1)
def __path_reader(element):
    """ A reader for Path objects in Version 1 scenes
    """
    traits = _read_base_shape_data(element)
    traits['svg_data'] = element.get('d')
    return PathModel(**traits)


@register_scene_writer(PathModel)
def __path_writer(write_func, model, parent):
    """ A writer for PathModel objects
    """
    element = SubElement(parent, NS_SVG + 'path')
    _write_base_shape_data(model, element)
    element.set('d', model.svg_data)
    return element


@register_scene_reader('Rectangle', xmltag=NS_SVG + 'rect', version=1)
def __rectangle_reader(element):
    """ A reader for Rectangle objects in Version 1 scenes
    """
    traits = _read_base_shape_data(element)
    traits.update(get_numbers(('x', 'y', 'width', 'height'), element))
    return RectangleModel(**traits)


@register_scene_writer(RectangleModel)
def __rectangle_writer(write_func, model, parent):
    """ A writer for RectangleModel objects
    """
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_shape_data(model, element)
    set_numbers(('x', 'y', 'width', 'height'), model, element)
    return element
