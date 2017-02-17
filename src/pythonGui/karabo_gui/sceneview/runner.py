from functools import partial
import os.path as op
import sys

from PyQt4.QtGui import QApplication

from karabo.common.scenemodel.api import read_scene
from .api import SceneView


def load_panel(scene_view):
    import karabo_gui.icons as icons
    from karabo_gui.panels.container import PanelContainer
    from karabo_gui.panels.scenepanel import ScenePanel

    icons.init()  # Very important!

    title = scene_view.scene_model.simple_name
    container = PanelContainer(title, None)
    factory = partial(ScenePanel, scene_view, True)
    container.addPanel(factory, title)
    return container


def main():
    app = QApplication(sys.argv)

    filename = sys.argv[1]

    model = read_scene(filename)
    scene_view = SceneView(model=model)
    widget = load_panel(scene_view)
    widget.show()
    widget.setWindowTitle(op.basename(filename))
    widget.resize(1024, 768)
    app.exec_()


if __name__ == '__main__':
    main()
