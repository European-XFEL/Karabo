#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QPainter, QSizePolicy, QWidget

from karabo_gui.scenemodel.api import (
    BoxLayoutModel, GridLayoutModel, LabelModel, LineModel, PathModel,
    RectangleModel, FixedLayoutModel, UnknownXMLDataModel, read_scene,
    SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT
)
from .const import QT_BOX_LAYOUT_DIRECTION
from .layouts import BoxLayout, GridLayout, GroupLayout
from .shapes import LineShape, PathShape, RectangleShape
from .simple_widgets import LabelWidget, UnknownSvgWidget


def fill_root_layout(layout, parent_model, scene_widget):
    # Go through children and create corresponding GUI objects
    for child in parent_model.children:
        if isinstance(child, FixedLayoutModel):
            obj = GroupLayout()
            layout.add_layout(obj, child)
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, BoxLayoutModel):
            obj = BoxLayout(QT_BOX_LAYOUT_DIRECTION[child.direction])
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, GridLayoutModel):
            obj = GridLayout()
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
            layout.add_widget(obj, child)
        if isinstance(child, UnknownXMLDataModel):
            obj = UnknownSvgWidget.create(child, parent=scene_widget)
            if obj is not None:
                layout.add_widget(obj, child)
            # XXX: UnknownXMLDataModel has a list of children, which might
            # include regular models. We're ignoring those children for now.


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.title = None
        self.designMode = designMode
        self.scene_model = None

        self.layout = GroupLayout(parent=self)

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

    def load(self, filename):
        """ The given ``filename`` is loaded.
        """
        # Set name
        self.title = os.path.basename(filename)
        # Read file into scene model
        self.scene_model = read_scene(filename)
        # Set width and height
        self.resize(max(self.scene_model.width, SCENE_MIN_WIDTH),
                    max(self.scene_model.height, SCENE_MIN_HEIGHT))
        fill_root_layout(self.layout, self.scene_model, self)

    def paintEvent(self, event):
        """ Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
