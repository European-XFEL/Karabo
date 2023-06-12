#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 7, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from collections import OrderedDict
from os import listdir
from os.path import isfile, join, splitext
from xml.etree.ElementTree import Element, parse, tostring

from karabo.common.scenemodel.const import NS_SVG


class ColorChangeIcon:
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

        root = Element(NS_SVG+'svg')
        root.append(self.element)
        return tostring(root, encoding='unicode')


def get_color_change_icon(path):
    """Icon from an SVG file which contain regions with a
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
        for group in element.findall(NS_SVG + 'g'):
            yield group
            yield from _iter_groups_r(group)

    tree = parse(path)

    try:
        icon = list(_iter_icon_groups(tree))[0]
    except IndexError:
        return None
    description = icon.findall(NS_SVG + 'desc')[0].text
    styleable = [(c, _split_style_attr(s))
                 for c, s in _iter_children_with_styles(icon)]
    white_subelements = [c for c, s in styleable
                         if s.get('fill', '').lower() == '#ffffff']
    return ColorChangeIcon(icon, white_subelements, description)


def get_color_change_icons(path):
    """
    Extract color change icons from SVG files located in `path`.
    """
    icons = {}
    for in_file in listdir(path):
        fpath = join(path, in_file)
        if isfile(fpath) and splitext(fpath)[-1] == '.svg':
            icon = get_color_change_icon(join(path, in_file))
            if icon is not None:
                icons[icon.name] = icon

    return icons

# ----------------------------------------------------------------------


def _join_style_attr(style):
    return ';'.join([f'{k}:{v}' for k, v in style.items()])


def _split_style_attr(style):
    return OrderedDict((k, v)
                       for k, v in (s.split(':') for s in style.split(';')))
