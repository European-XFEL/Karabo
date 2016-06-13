from karabo_gui.scenemodel.api import (
    BoxLayoutModel, GridLayoutModel, LabelModel, LineModel, PathModel,
    RectangleModel, FixedLayoutModel, UnknownXMLDataModel
)
from .const import QT_BOX_LAYOUT_DIRECTION
from .layouts import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .simple_widgets import LabelWidget, UnknownSvgWidget

_LAYOUT_CLASSES = (BoxLayout, GridLayout, GroupLayout)
_SHAPE_CLASSES = (LineShape, PathShape, RectangleShape)
_WIDGET_CLASSES = (LabelWidget, UnknownSvgWidget)
_SCENE_OBJ_FACTORIES = {
    FixedLayoutModel: lambda m, p: GroupLayout(m),
    BoxLayoutModel: lambda m, p: BoxLayout(m, QT_BOX_LAYOUT_DIRECTION[m.direction]),  # noqa
    GridLayoutModel: lambda m, p: GridLayout(m),
    LineModel: lambda m, p: LineShape(m),
    RectangleModel: lambda m, p: RectangleShape(m),
    PathModel: lambda m, p: PathShape(m),
    LabelModel: lambda m, p: LabelWidget(m, p),
    UnknownXMLDataModel: lambda m, p: UnknownSvgWidget.create(m, parent=p),
}


def fill_root_layout(layout, parent_model, scene_view, object_dict):
    """ Recursively build scene GUI objects for a given parent model object.
    Whenever a layout is encountered, its children are then added recursively.

    `object_dict` is a cache of already created GUI objects.
    """
    for child_model in parent_model.children:
        create_object_from_model(layout, child_model, scene_view, object_dict)


def create_object_from_model(layout, model, scene_view, object_dict):
    obj = object_dict.get(model)
    if obj is None:
        factory = _SCENE_OBJ_FACTORIES.get(model.__class__)
        if factory:
            obj = factory(model, scene_view)

    # Add the new scene object to the layout
    if obj is not None:
        if model not in object_dict:
            object_dict[model] = obj
        if is_layout(obj):
            layout.add_layout(obj)
            # recurse
            fill_root_layout(obj, model, scene_view, object_dict)
        elif is_shape(obj):
            layout.add_shape(obj)
        elif is_widget(obj):
            layout.add_widget(obj)


def is_layout(scene_obj):
    """Returns True if `scene_obj` is a layout."""
    return isinstance(scene_obj, _LAYOUT_CLASSES)


def is_shape(scene_obj):
    """Returns True if `scene_obj` is a shape."""
    return isinstance(scene_obj, _SHAPE_CLASSES)


def is_widget(scene_obj):
    """Returns True if `scene_obj` is a widget."""
    return isinstance(scene_obj, _WIDGET_CLASSES)
