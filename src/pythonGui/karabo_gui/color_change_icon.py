#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, parse, tostring

from karabo_gui.const import ns_svg, ns_inkscape


class ColorChangeIcon(object):
    """An SVG group element which can have fill colors changed in one or more
    subelements.
    """
    def __init__(self, element, white_filled_subelements):
        self.element = element
        self.name = element.get(ns_inkscape + 'label')
        self.white_filled_subelements = white_filled_subelements

    def with_color(self, color):
        """ Return the XML for the icon with white fill colors replaced
        """
        for element in self.white_filled_subelements:
            style = _split_style_attr(element.get('style'))
            style['fill'] = color
            element.set('style', _join_style_attr(style))

        root = Element(ns_svg+'svg')
        root.append(self.element)
        return tostring(root, encoding='utf-8')


def get_color_change_icons(path):
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
            label = group.get(ns_inkscape + 'label')
            if label is not None and label.startswith('icon'):
                yield group

    def _iter_groups_r(element):
        for group in element.findall(ns_svg + 'g'):
            yield group
            for subgroup in _iter_groups_r(group):
                yield subgroup

    def _iter_changeable_icons(root):
        for icon in _iter_icon_groups(root):
            styleable = [(c, _split_style_attr(s))
                         for c, s in _iter_children_with_styles(icon)]
            white_subelements = [c for c, s in styleable
                                 if s.get('fill', '').lower() == '#ffffff']
            if white_subelements:
                yield icon, white_subelements

    tree = parse(path)
    icons = [ColorChangeIcon(icon, white_subelements)
             for icon, white_subelements in _iter_changeable_icons(tree)]
    return {icon.name: icon for icon in icons}

# ----------------------------------------------------------------------


def _join_style_attr(style):
    return ';'.join(['{}:{}'.format(k, v) for k, v in style.items()])


def _split_style_attr(style):
    return {k: v for k, v in (s.split(':') for s in style.split(';'))}
