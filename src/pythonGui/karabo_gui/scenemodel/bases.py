from traits.api import (HasTraits, Enum, Instance, Int, Float, List, Range,
                        String)


class BaseLayoutData(HasTraits):
    """ An empty base class to simplify holding layout data in other objects.
    """


class BaseSceneObjectData(HasTraits):
    """ A base class for all object which can appear in a scene.
    """
    # The data needed by the layout which is the parent for this object
    layout_data = Instance(BaseLayoutData)


class BaseLayoutModel(BaseSceneObjectData):
    """ A common base class for all layouts
    """
    # The X-coordinate of the layout
    x = Float
    # The Y-coordinate of the layout
    y = Float
    # The height of the layout
    height = Float
    # The width of the layout
    width = Float
    # The children of the layout
    children = List(Instance(BaseSceneObjectData))


class BaseShapeObjectData(BaseSceneObjectData):
    """ Base class for "shape" objects which contains drawing style attributes
    """
    # An HTML color #hex value, or "none"
    stroke = String('none')
    # A floating point number between 0 and 1. Default 1
    stroke_opacity = Range(low=0.0, high=1.0, value=1.0)
    # butt, square, or round. Default butt
    stroke_linecap = Enum('butt', 'square', 'round')
    # A floating point number. Default 0
    stroke_dashoffset = Float(0.0)
    # A floating point number. Default 1
    stroke_width = Float(1.0)
    # A comma-separated list of dash lengths. Default "none"
    stroke_dasharray = List(Float)
    # An integer which maps to a QPen style. Default Qt.SolidLine (ie: 1)
    stroke_style = Int(1)
    # miter, round, or bevel. Default miter
    stroke_linejoin = Enum('miter', 'round', 'bevel')
    # A floating point number. Default 4
    stroke_miterlimit = Float(4.0)
    # An HTML color #hex value, or “none”
    fill = String('none')
    # A floating point number between 0 and 1. Default 1
    fill_opacity = Range(low=0.0, high=1.0, value=1.0)


class BaseWidgetObjectData(BaseSceneObjectData):
    """ A base class for all widgets
    """
    # The property names viewed by the widget
    keys = List(String)
    # The possible component type for a parent of the widget (can be empty)
    parent_component = String
    # The X-coordinate of the widget
    x = Float
    # The Y-coordinate of the widget
    y = Float
    # The height of the widget
    height = Float
    # The width of the widget
    width = Float
