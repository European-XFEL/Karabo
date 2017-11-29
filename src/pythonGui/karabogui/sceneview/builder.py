from PyQt4.QtCore import QRect

from karabo.common.scenemodel import api as models
from karabogui.controllers.registry import get_model_controller
from .const import QT_BOX_LAYOUT_DIRECTION
from .layout.api import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .widget.api import (ControllerContainer, LabelWidget, SceneLinkWidget,
                         UnknownSvgWidget, UnknownWidget, WorkflowItemWidget)

_LAYOUT_CLASSES = (BoxLayout, GridLayout, GroupLayout)
_SHAPE_CLASSES = (LineShape, PathShape, RectangleShape)
_WIDGET_CLASSES = (ControllerContainer, LabelWidget, SceneLinkWidget,
                   UnknownSvgWidget, UnknownWidget, WorkflowItemWidget)
_SCENE_OBJ_FACTORIES = {
    models.BoxLayoutModel: lambda m, p: BoxLayout(m, QT_BOX_LAYOUT_DIRECTION[m.direction]),  # noqa
    models.FixedLayoutModel: lambda m, p: GroupLayout(m),
    models.GridLayoutModel: lambda m, p: GridLayout(m),
    models.LineModel: lambda m, p: LineShape(model=m),
    models.PathModel: lambda m, p: PathShape(model=m),
    models.RectangleModel: lambda m, p: RectangleShape(model=m),
    models.UnknownXMLDataModel: lambda m, p: UnknownSvgWidget.create(m, parent=p),  # noqa
}


def find_top_level_model(scene_model, model):
    """Recursively find the model in the ``parent_model`` tree which matches
    the given ``model`` and return its parent model which is the top level
    model in the end.
    """
    def recurse(parent_model, seek_model):
        if isinstance(parent_model, models.BaseLayoutModel):
            for child in parent_model.children:
                result = recurse(child, seek_model)
                if result is not None:
                    return parent_model

        return parent_model if parent_model is seek_model else None

    for child in scene_model.children:
        top_level_model = recurse(child, model)
        if top_level_model is not None:
            return top_level_model


def replace_model_in_top_level_model(layout_model, parent_model, old_model,
                                     new_model):
    """Recursively find the given ``old_model`` in the model tree and replace
    it with the given ``new_model``.

    This method returns ``True`` if the model was replaced.
    """
    if isinstance(parent_model, models.BaseLayoutModel):
        for child in parent_model.children:
            result = replace_model_in_top_level_model(parent_model, child,
                                                      old_model, new_model)
            if result:
                return True
    elif parent_model is old_model:
        # Replace old model with new model
        layout_children = layout_model.children
        index = layout_children.index(parent_model)
        layout_children.remove(parent_model)
        layout_children.insert(index, new_model)
        # Enforce recalculation of geometry
        layout_model.width = 0
        layout_model.height = 0
        return True
    return False


def add_object_to_layout(obj, layout):
    """Add a SceneView object to a layout"""
    if is_shape(obj):
        layout._add_shape(obj)
    elif is_widget(obj):
        layout._add_widget(obj)
    elif is_layout(obj):
        layout._add_layout(obj)
    # Make sure the object is showing
    obj.show()


def remove_object_from_layout(obj, layout, object_dict, scene_visible):
    """Remove a SceneView object from a layout"""
    if is_shape(obj):
        layout._remove_shape(obj)
    elif is_widget(obj):
        layout._remove_widget(obj)
        if scene_visible:
            obj.set_visible(False)
    elif is_layout(obj):
        layout._remove_layout(obj)
        # Recursively remove all children
        for model in obj.model.children:
            child_obj = object_dict.get(model)
            if child_obj:
                remove_object_from_layout(child_obj, obj, object_dict,
                                          scene_visible)

    # Hide object from scene until reparenting is done
    obj.hide()


def create_object_from_model(layout, model, parent_widget, object_dict,
                             scene_visible):
    """Create a SceneView object to mirror a data model object"""
    obj = object_dict.get(model)
    if obj is None:
        factory = _SCENE_OBJ_FACTORIES.get(model.__class__)
        if factory:
            obj = factory(model, parent_widget)
        else:
            klass = get_model_controller(model)
            obj = ControllerContainer(klass, model, parent_widget)
        # FIXME: Handle wiget deprecations here!

    # Add the new scene object to the layout
    if obj is not None:
        if model not in object_dict:
            object_dict[model] = obj
        add_object_to_layout(obj, layout)
        if is_layout(obj):
            # recurse
            fill_root_layout(obj, model, parent_widget, object_dict,
                             scene_visible)
            model_rect = QRect(model.x, model.y, model.width, model.height)
            if model_rect.isEmpty():
                # Ask the layout to calculate a suitable size
                obj.invalidate()
                rect = obj.geometry()
                if rect.isEmpty():
                    model_rect.setSize(obj.sizeHint())
                else:
                    model_rect = rect
            obj.setGeometry(model_rect)
        elif is_widget(obj):
            model_rect = QRect(model.x, model.y, model.width, model.height)
            widget_rect = obj.geometry()
            if model_rect.isEmpty():
                model_rect.setSize(obj.sizeHint())
                obj.set_geometry(model_rect)
            elif not widget_rect.isEmpty() and widget_rect != model_rect:
                # Handle the case when widgets unparented from a layout have
                # geometry which disagrees with their model!
                obj.set_geometry(widget_rect)
            # Don't just call set_visible(scene_visible) here!
            # The visibility count will be wrong if scene_visible is False!
            if scene_visible:
                obj.set_visible(True)


def fill_root_layout(layout, parent_model, parent_widget, object_dict,
                     scene_visible):
    """Recursively build scene GUI objects for a given parent model object.
    Whenever a layout is encountered, its children are then added recursively.

    `object_dict` is a cache of already created GUI objects.
    """
    for child_model in parent_model.children:
        create_object_from_model(layout, child_model, parent_widget,
                                 object_dict, scene_visible)


def bring_object_to_front(obj):
    if is_widget(obj):
        obj.raise_()
    elif is_layout(obj):
        for child in obj:
            layout = child.layout()
            if layout is not None:
                bring_object_to_front(layout)
                continue
            widget = child.widget()
            if widget is not None:
                bring_object_to_front(widget)


def send_object_to_back(obj):
    if is_widget(obj):
        obj.lower()
    elif is_layout(obj):
        for child in obj:
            layout = child.layout()
            if layout is not None:
                send_object_to_back(layout)
                continue
            widget = child.widget()
            if widget is not None:
                send_object_to_back(widget)


def is_layout(scene_obj):
    """Returns True if `scene_obj` is a layout"""
    return isinstance(scene_obj, _LAYOUT_CLASSES)


def is_shape(scene_obj):
    """Returns True if `scene_obj` is a shape"""
    return isinstance(scene_obj, _SHAPE_CLASSES)


def is_widget(scene_obj):
    """Returns True if `scene_obj` is a widget"""
    return isinstance(scene_obj, _WIDGET_CLASSES)
