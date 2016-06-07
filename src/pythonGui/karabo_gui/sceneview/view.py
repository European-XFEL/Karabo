#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QPainter, QSizePolicy, QWidget

from karabo_gui.scenemodel.api import (LabelModel, LineModel, RectangleModel,
                                       read_scene, SCENE_MIN_WIDTH,
                                       SCENE_MIN_HEIGHT)
from .layouts import BaseLayout
from .shapes import Label, LineShape, RectangleShape


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.filename = None
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
        # Set name
        self.filename = os.path.basename(filename)
        # Read file into scene model
        self.scene_model = read_scene(filename)
        # Set width and height
        self.resize(self.scene_model.width, self.scene_model.height)

        # Go through children and create corresponding GUI objects
        for child in self.scene_model.children:
            if isinstance(child, LineModel):
                obj = LineShape(child)
                self.layout.add_shape(obj)
            if isinstance(child, RectangleModel):
                obj = RectangleShape(child)
                self.layout.add_shape(obj)
            if isinstance(child, LabelModel):
                obj = Label(child)
                self.layout.add_shape(obj)

    def paintEvent(self, event):
        """ Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
