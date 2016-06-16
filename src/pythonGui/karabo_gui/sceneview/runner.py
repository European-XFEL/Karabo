import os.path as op
import sys

from PyQt4.QtGui import QApplication, QFrame, QHBoxLayout, QVBoxLayout

from .api import SceneView


def load_panel(scene_view):
    import karabo_gui.icons as icons
    from karabo_gui.panels.scenepanel import ScenePanel
    from karabo_gui.toolbar import ToolBar

    class _Frame(QFrame):
        def __init__(self, panel):
            super(_Frame, self).__init__()
            self.toolbar = ToolBar()
            self.dockableWidget = panel

            self.toolBarLayout = QHBoxLayout()
            self.toolBarLayout.setContentsMargins(0, 0, 0, 0)
            self.toolBarLayout.setSpacing(0)
            self.addToolBar(self.toolbar)

            vLayout = QVBoxLayout(self)
            vLayout.setContentsMargins(0, 0, 0, 0)
            vLayout.addLayout(self.toolBarLayout)
            vLayout.addWidget(self.dockableWidget)

            self.dockableWidget.setupToolBars(self.toolbar, self)

        def addToolBar(self, toolbar):
            self.toolBarLayout.addWidget(toolbar)

    icons.init()  # Very important!
    return _Frame(ScenePanel(scene_view, True))


def main():
    app = QApplication(sys.argv)

    filename = sys.argv[1]

    scene_view = SceneView()
    scene_view.load(filename)
    widget = load_panel(scene_view)
    widget.show()
    widget.setWindowTitle(op.basename(filename))
    widget.resize(1024, 768)
    app.exec_()

if __name__ == '__main__':
    main()
