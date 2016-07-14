from PyQt4.QtCore import QRect

from karabo_gui.scenemodel.api import (
    BaseLayoutModel, BoxLayoutModel, FixedLayoutModel, GridLayoutModel,
    LabelModel, LineModel, PathModel, RectangleModel, SceneLinkModel,
    UnknownXMLDataModel, BitfieldModel, DisplayAlignedImageModel,
    DisplayCommandModel, DisplayIconsetModel, DisplayImageModel,
    DisplayImageElementModel, DisplayLabelModel, DisplayPlotModel,
    DoubleLineEditModel, EditableListModel, EditableListElementModel,
    EditableSpinBoxModel, HexadecimalModel, IntLineEditModel, KnobModel,
    SliderModel, XYPlotModel, CheckBoxModel, ChoiceElementModel, ComboBoxModel,
    DirectoryModel, FileInModel, FileOutModel, LineEditModel,
    DisplayStateColorModel, DigitIconsModel, EvaluatorModel, FloatSpinBoxModel,
    LinePlotModel, MonitorModel, SelectionIconsModel, SingleBitModel,
    TableElementModel, TextIconsModel, VacuumWidgetModel, WorkflowItemModel
)
from .const import QT_BOX_LAYOUT_DIRECTION
from .layout.api import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .widget.api import (
    BaseWidgetContainer, DisplayEditableWidgetContainer,
    DisplayStateColorContainer, EvaluatorContainer, FloatSpinBoxContainer,
    GenericWidgetContainer, IconsContainer, LabelWidget, LinePlotContainer,
    MonitorContainer, SceneLinkWidget, SingleBitContainer,
    TableElementContainer, UnknownSvgWidget, VacuumWidgetContainer,
    WorkflowItemWidget)

_LAYOUT_CLASSES = (BoxLayout, GridLayout, GroupLayout)
_SHAPE_CLASSES = (LineShape, PathShape, RectangleShape)
_WIDGET_CLASSES = (
    BaseWidgetContainer, LabelWidget, SceneLinkWidget, UnknownSvgWidget)
_SCENE_OBJ_FACTORIES = {
    FixedLayoutModel: lambda m, p: GroupLayout(m),
    BoxLayoutModel: lambda m, p: BoxLayout(m, QT_BOX_LAYOUT_DIRECTION[m.direction]),  # noqa
    GridLayoutModel: lambda m, p: GridLayout(m),
    LineModel: lambda m, p: LineShape(model=m),
    RectangleModel: lambda m, p: RectangleShape(model=m),
    PathModel: lambda m, p: PathShape(model=m),
    LabelModel: LabelWidget,
    SceneLinkModel: SceneLinkWidget,
    UnknownXMLDataModel: lambda m, p: UnknownSvgWidget.create(m, parent=p),
    BitfieldModel: GenericWidgetContainer,
    DisplayAlignedImageModel: GenericWidgetContainer,
    DisplayCommandModel: GenericWidgetContainer,
    DisplayIconsetModel: GenericWidgetContainer,
    DisplayImageModel: GenericWidgetContainer,
    DisplayImageElementModel: GenericWidgetContainer,
    DisplayLabelModel: GenericWidgetContainer,
    DisplayPlotModel: GenericWidgetContainer,
    DoubleLineEditModel: GenericWidgetContainer,
    EditableListModel: GenericWidgetContainer,
    EditableListElementModel: GenericWidgetContainer,
    EditableSpinBoxModel: GenericWidgetContainer,
    HexadecimalModel: GenericWidgetContainer,
    IntLineEditModel: GenericWidgetContainer,
    KnobModel: GenericWidgetContainer,
    SliderModel: GenericWidgetContainer,
    XYPlotModel: GenericWidgetContainer,
    CheckBoxModel: DisplayEditableWidgetContainer,
    ChoiceElementModel: DisplayEditableWidgetContainer,
    ComboBoxModel: DisplayEditableWidgetContainer,
    DirectoryModel: DisplayEditableWidgetContainer,
    FileInModel: DisplayEditableWidgetContainer,
    FileOutModel: DisplayEditableWidgetContainer,
    LineEditModel: DisplayEditableWidgetContainer,
    DisplayStateColorModel: DisplayStateColorContainer,
    DigitIconsModel: IconsContainer,
    SelectionIconsModel: IconsContainer,
    TextIconsModel: IconsContainer,
    VacuumWidgetModel: VacuumWidgetContainer,
    EvaluatorModel: EvaluatorContainer,
    FloatSpinBoxModel: FloatSpinBoxContainer,
    SingleBitModel: SingleBitContainer,
    MonitorModel: MonitorContainer,
    LinePlotModel: LinePlotContainer,
    TableElementModel: TableElementContainer,
    WorkflowItemModel: WorkflowItemWidget,
}


def find_top_level_model(parent_model, model):
    """ Recursively find the model in the ``parent_model`` tree which matches
        the given ``model`` and return its parent model which is the top level
        model in the end.
    """
    if isinstance(parent_model, BaseLayoutModel):
        for child in parent_model.children:
            result = find_top_level_model(child, model)
            if result:
                return parent_model

    return parent_model if parent_model is model else None


def replace_model_in_top_level_model(layout_model, parent_model, old_model,
                                     new_model):
    """ Recursively find the given ``old_model`` in the model tree and
        replace it with the given ``new_model``.

        This method returns, if replacing was successful.
    """
    if isinstance(parent_model, BaseLayoutModel):
        for child in parent_model.children:
            result = replace_model_in_top_level_model(parent_model, child,
                                                      old_model, new_model)
            if result:
                return True
    elif parent_model is old_model:
        # Replace old model with new model
        layout_children = layout_model.children
        try:
            index = layout_children.index(parent_model)
            layout_children.remove(parent_model)
            layout_children.insert(index, new_model)
        except ValueError:
            # Model already removed
            return False
        # Enforce recalculation of geometry
        layout_model.width = 0
        layout_model.height = 0
        return True
    return False


def add_object_to_layout(obj, layout):
    """ Add a SceneView object to a layout.
    """
    if is_shape(obj):
        layout._add_shape(obj)
    elif is_widget(obj):
        layout._add_widget(obj)
    elif is_layout(obj):
        layout._add_layout(obj)
    # Make sure the object is showing
    obj.show()


def remove_object_from_layout(obj, layout, object_dict):
    """ Remove a SceneView object from a layout.
    """
    if is_shape(obj):
        layout._remove_shape(obj)
    elif is_widget(obj):
        layout._remove_widget(obj)
    elif is_layout(obj):
        layout._remove_layout(obj)
        # Recursively remove all children
        for model in obj.model.children:
            child_obj = object_dict.get(model)
            if child_obj:
                remove_object_from_layout(child_obj, obj, object_dict)

    # Hide object from scene until reparenting is done
    obj.hide()


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
            if model_rect.isEmpty():
                model_rect.setSize(obj.sizeHint())
                obj.setGeometry(model_rect)


def fill_root_layout(layout, parent_model, scene_view, object_dict):
    """ Recursively build scene GUI objects for a given parent model object.
    Whenever a layout is encountered, its children are then added recursively.

    `object_dict` is a cache of already created GUI objects.
    """
    for child_model in parent_model.children:
        create_object_from_model(layout, child_model, scene_view, object_dict)


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
    """Returns True if `scene_obj` is a layout."""
    return isinstance(scene_obj, _LAYOUT_CLASSES)


def is_shape(scene_obj):
    """Returns True if `scene_obj` is a shape."""
    return isinstance(scene_obj, _SHAPE_CLASSES)


def is_widget(scene_obj):
    """Returns True if `scene_obj` is a widget."""
    return isinstance(scene_obj, _WIDGET_CLASSES)
