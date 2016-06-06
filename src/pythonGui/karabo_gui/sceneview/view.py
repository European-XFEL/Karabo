#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QPainter, QSizePolicy, QWidget

from .layouts import BaseLayout
from karabo_gui.scenemodel.api import (LabelModel, LineModel, RectangleModel,
                                       read_scene, SCENE_MIN_WIDTH,
                                       SCENE_MIN_HEIGHT)
from .shapes import Label, Line, Rectangle


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, name, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.filename = name
        self.designMode = designMode
        self.scene_model = None

        self.layout = BaseLayout()

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

    def load(self, filename):
        """ The given ``filename`` is loaded.
        """
        self.scene_model = read_scene(filename)
        # Set width and height
        self.resize(self.scene_model.width, self.scene_model.height)

        # Go through children and create corresponding GUI objects
        for child in self.scene_model.children:
            if isinstance(child, LineModel):
                obj = Line(child)
                self.layout.add_shape(obj)
            if isinstance(child, RectangleModel):
                obj = Rectangle(child)
                self.layout.add_shape(obj)
            if isinstance(child, LabelModel):
                obj = Label(child)
                self.layout.add_shape(obj)

    def paintEvent(self, event):
        """ Show view content.
        """
        painter = QPainter(self)
        try:
            self.layout.draw(painter)
        finally:
            painter.end()
