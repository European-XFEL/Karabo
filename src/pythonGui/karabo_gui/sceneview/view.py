#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QSizePolicy, QWidget

from karabo_gui.scenemodel.api import (read_scene, SceneModel, SCENE_MIN_WIDTH,
                                       SCENE_MIN_HEIGHT)


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, name, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.filename = name
        self.designMode = designMode
        self.scene_model = None

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
