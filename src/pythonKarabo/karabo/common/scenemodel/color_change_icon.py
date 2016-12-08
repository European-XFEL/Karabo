#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, parse, tostring
from os import listdir
from os.path import join, isfile

from karabo_gui.const import ns_svg, ns_inkscape


class ColorChangeIcon(object):
    """An SVG group element which can have fill colors changed in one or more
    subelements.
    """
    def __init__(self, element, white_filled_subelements, description):
        self.element = element
        self.name = element.get('id')
        self.white_filled_subelements = white_filled_subelements
        self.description = description

    def with_color(self, color):
        """ Return the XML for the icon with white fill colors replaced
        """

        # make sure color is in hex representation
        if isinstance(color, (tuple, list)):
            color = '#%02x%02x%02x' % color[:3]

        for element in self.white_filled_subelements:
            style = _split_style_attr(element.get('style'))
            style['fill'] = color
            element.set('style', _join_style_attr(style))

        root = Element(ns_svg+'svg')
        root.append(self.element)
        return tostring(root, encoding='unicode')


def _get_color_change_icon(path):
    """Extract all the icons from and SVG file which contain regions with a
    fill color of white.

    :param path: A path to an SVG file
    :return: A list of ``ColorChangeIcon`` instances
    """
    def _iter_children_with_styles(element):
        for child in element:
            style = child.get('style')
            if style is not None:
                yield child, style

    def _iter_icon_groups(root):
        for group in _iter_groups_r(root):
            label = group.get('id')
            if label is not None and label.startswith('icon'):
                yield group

    def _iter_groups_r(element):
        for group in element.findall(ns_svg + 'g'):
            yield group
            for subgroup in _iter_groups_r(group):
                yield subgroup

    def _iter_changeable_icons(root):
        for icon in _iter_icon_groups(root):
            description = icon.findall(ns_svg +'desc')[0].text
            styleable = [(c, _split_style_attr(s))
                         for c, s in _iter_children_with_styles(icon)]
            white_subelements = [c for c, s in styleable
                                 if s.get('fill', '').lower() == '#ffffff']
            if white_subelements:
                yield icon, white_subelements, description

    tree = parse(path)
    icons = [ColorChangeIcon(icon, white_subelements, description)
             for icon, white_subelements, description
                in _iter_changeable_icons(tree)]
    return {icon.name: icon for icon in icons}

def get_color_change_icons(path):
    """
    Extract color change icons from SVG files located in `path`.
    """
    icons = {}
    for in_file in listdir(path):
        if isfile(join(path, in_file)):
            icon = _get_color_change_icon(join(path, in_file))
            for k, i in icon.items():
                icons[k] = i

    return icons

# ----------------------------------------------------------------------


def _join_style_attr(style):
    return ';'.join(['{}:{}'.format(k, v) for k, v in style.items()])


def _split_style_attr(style):
    return {k: v for k, v in (s.split(':') for s in style.split(';'))}
