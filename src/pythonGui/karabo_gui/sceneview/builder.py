from karabo_gui.scenemodel.api import (
    BoxLayoutModel, FixedLayoutModel, GridLayoutModel, LabelModel, LineModel,
    PathModel, RectangleModel, SceneLinkModel, UnknownXMLDataModel
)
from .const import QT_BOX_LAYOUT_DIRECTION
from .layouts import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .simple_widgets import LabelWidget, SceneLinkWidget, UnknownSvgWidget

_LAYOUT_CLASSES = (BoxLayout, GridLayout, GroupLayout)
_SHAPE_CLASSES = (LineShape, PathShape, RectangleShape)
_WIDGET_CLASSES = (LabelWidget, SceneLinkWidget, UnknownSvgWidget)
_SCENE_OBJ_FACTORIES = {
    FixedLayoutModel: lambda m, p: GroupLayout(m),
    BoxLayoutModel: lambda m, p: BoxLayout(m, QT_BOX_LAYOUT_DIRECTION[m.direction]),  # noqa
    GridLayoutModel: lambda m, p: GridLayout(m),
    LineModel: lambda m, p: LineShape(m),
    RectangleModel: lambda m, p: RectangleShape(m),
    PathModel: lambda m, p: PathShape(m),
    LabelModel: lambda m, p: LabelWidget(m, p),
    SceneLinkModel: lambda m, p: SceneLinkWidget(m, p),
    UnknownXMLDataModel: lambda m, p: UnknownSvgWidget.create(m, parent=p),
}


def add_object_to_layout(obj, layout):
    """ Add a SceneView object to a layout.
    """
    if is_shape(obj):
        layout._add_shape(obj)
    elif is_widget(obj):
        layout._add_widget(obj)
    elif is_layout(obj):
        layout._add_layout(obj)


def remove_object_from_layout(obj, layout):
    """ Remove a SceneView object from a layout.
    """
    if is_shape(obj):
        layout._remove_shape(obj)
    elif is_widget(obj):
        layout._remove_widget(obj)
    elif is_layout(obj):
        layout._remove_layout(obj)


def create_object_from_model(layout, model, scene_view, object_dict):
    """ Create a SceneView object to mirror a data model object.
    """
    obj = object_dict.get(model)
    if obj is None:
        factory = _SCENE_OBJ_FACTORIES.get(model.__class__)
        if factory:
            obj = factory(model, scene_view)

    # Add the new scene object to the layout
    if obj is not None:
        if model not in object_dict:
            object_dict[model] = obj
        add_object_to_layout(obj, layout)
        if is_layout(obj):
            # recurse
            fill_root_layout(obj, model, scene_view, object_dict)


def fill_root_layout(layout, parent_model, scene_view, object_dict):
    """ Recursively build scene GUI objects for a given parent model object.
    Whenever a layout is encountered, its children are then added recursively.

    `object_dict` is a cache of already created GUI objects.
    """
    for child_model in parent_model.children:
        create_object_from_model(layout, child_model, scene_view, object_dict)


def is_layout(scene_obj):
    """Returns True if `scene_obj` is a layout."""
    return isinstance(scene_obj, _LAYOUT_CLASSES)


def is_shape(scene_obj):
    """Returns True if `scene_obj` is a shape."""
    return isinstance(scene_obj, _SHAPE_CLASSES)


def is_widget(scene_obj):
    """Returns True if `scene_obj` is a widget."""
    return isinstance(scene_obj, _WIDGET_CLASSES)
