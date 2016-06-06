
class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, project, name, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.project = project
        self.filename = name
        self.designMode = designMode

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        #self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)
