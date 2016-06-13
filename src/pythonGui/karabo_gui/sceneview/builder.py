from karabo_gui.scenemodel.api import (
    BoxLayoutModel, GridLayoutModel, LabelModel, LineModel, PathModel,
    RectangleModel, FixedLayoutModel, UnknownXMLDataModel
)
from .const import QT_BOX_LAYOUT_DIRECTION
from .layouts import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .simple_widgets import LabelWidget, UnknownSvgWidget


def fill_root_layout(layout, parent_model, scene_widget):
    # Go through children and create corresponding GUI objects
    for child in parent_model.children:
        if isinstance(child, FixedLayoutModel):
            obj = GroupLayout(child)
            layout.add_layout(obj)
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, BoxLayoutModel):
            obj = BoxLayout(child, QT_BOX_LAYOUT_DIRECTION[child.direction])
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, GridLayoutModel):
            obj = GridLayout(child)
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, LineModel):
            obj = LineShape(child)
            layout.add_shape(obj)
        if isinstance(child, RectangleModel):
            obj = RectangleShape(child)
            layout.add_shape(obj)
        if isinstance(child, PathModel):
            obj = PathShape(child)
            layout.add_shape(obj)
        if isinstance(child, LabelModel):
            obj = LabelWidget(child, scene_widget)
            layout.add_widget(obj)
        if isinstance(child, UnknownXMLDataModel):
            obj = UnknownSvgWidget.create(child, parent=scene_widget)
            if obj is not None:
                layout.add_widget(obj)
            # XXX: UnknownXMLDataModel has a list of children, which might
            # include regular models. We're ignoring those children for now.
