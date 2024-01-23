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
from random import randint

from traits.api import CInt, Enum, Float, Instance, Int, List, Range, String

from karabo.common.api import BaseSavableModel

from .const import DISPLAY_COMPONENT, EDITABLE_COMPONENT


class BaseLayoutData(BaseSavableModel):
    """An empty base class to simplify holding layout data in other objects."""


class BaseSceneObjectData(BaseSavableModel):
    """A base class for all object which can appear in a scene."""

    # The data needed by the layout which is the parent for this object
    layout_data = Instance(BaseLayoutData)


class BaseLayoutModel(BaseSceneObjectData):
    """A common base class for all layouts"""

    # The X-coordinate of the layout
    x = CInt
    # The Y-coordinate of the layout
    y = CInt
    # The height of the layout
    height = CInt
    # The width of the layout
    width = CInt
    # The children of the layout
    children = List(Instance(BaseSceneObjectData))


class BaseShapeObjectData(BaseSceneObjectData):
    """Base class for shape objects which contains drawing style attributes"""

    # An HTML color #hex value, or "none"
    stroke = String("none")
    # A floating point number between 0 and 1. Default 1
    stroke_opacity = Range(low=0.0, high=1.0, value=1.0)
    # butt, square, or round. Default butt
    stroke_linecap = Enum("butt", "square", "round")
    # A floating point number. Default 0
    stroke_dashoffset = Float(0.0)
    # A floating point number. Default 1
    stroke_width = Float(1.0)
    # A comma-separated list of dash lengths. Default "none"
    stroke_dasharray = List(Float)
    # An integer which maps to a QPen style. Default Qt.SolidLine (ie: 1)
    stroke_style = Int(1)
    # miter, round, or bevel. Default miter
    stroke_linejoin = Enum("miter", "round", "bevel")
    # A floating point number. Default 4
    stroke_miterlimit = Float(4.0)
    # An HTML color #hex value, or "none"
    fill = String("none")
    # A floating point number between 0 and 1. Default 1
    fill_opacity = Range(low=0.0, high=1.0, value=1.0)


class BaseWidgetObjectData(BaseSceneObjectData):
    """A base class for all controllers"""

    # The property names viewed by the controller
    keys = List(String)
    # The possible component type for a parent of the controller
    parent_component = String
    # The X-coordinate of the controller
    x = CInt
    # The Y-coordinate of the controller
    y = CInt
    # The height of the controller
    height = CInt
    # The width of the controller
    width = CInt

    def _parent_component_default(self):
        """If this method is not overridden by a derived class, return the
        default value of 'DisplayComponent'
        """
        return DISPLAY_COMPONENT


class BaseEditWidget(BaseWidgetObjectData):
    """The base class for all controllers which can edit"""

    def _parent_component_default(self):
        return EDITABLE_COMPONENT


class BaseDisplayEditableWidget(BaseWidgetObjectData):
    """Handle the value of the `parent_component` trait for models which
    represent both display and editable controllers.
    """

    def _parent_component_default(self):
        if self.klass.startswith("Editable"):
            return EDITABLE_COMPONENT
        return DISPLAY_COMPONENT


class XMLElementModel(BaseSceneObjectData):
    """The base class for XML(SVG) items"""

    # The id attribute
    id = String

    def _id_default(self):
        return self.generate_id()

    def reset_id(self):
        self.id = self.generate_id()

    def generate_id(self):
        """Generate a random ID specified on the subclass"""
        return ""

    def randomize(self, text):
        return f"{text}{randint(0, int(1e6))}"
