import sys

from PyQt4.QtGui import QApplication

from karabo.common.scenemodel.api import read_scene
import karabo_gui.icons as icons
from karabo_gui.panels.scenepanel import ScenePanel


def main():
    app = QApplication(sys.argv)

    icons.init()  # Very important!

    filename = sys.argv[1]

    model = read_scene(filename)
    panel = ScenePanel(model, True)
    panel.show()
    panel.resize(1024, 768)
    app.exec_()


if __name__ == '__main__':
    main()
