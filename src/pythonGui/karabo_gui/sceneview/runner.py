import sys

from PyQt4.QtGui import QApplication

from karabo.common.scenemodel.api import read_scene
import karabo_gui.icons as icons
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.singletons.api import get_manager, get_network


def main():
    app = QApplication(sys.argv)

    icons.init()  # Very important!

    filename = sys.argv[1]

    model = read_scene(filename)
    panel = ScenePanel(model, True)
    # XXX: A hack to keep the toolbar visible
    panel.toolbar.setVisible(True)
    panel.show()
    panel.resize(1024, 768)

    # XXX: A hack to connect to the GUI Server for testing
    get_manager()
    get_network().connectToServer()

    app.exec_()


if __name__ == '__main__':
    main()
