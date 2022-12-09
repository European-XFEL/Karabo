from pathlib import Path

from karabo.common.scenemodel.api import read_scene
from karabogui.sceneview.api import SceneView

DATA_DIR = Path(__file__).parent.joinpath("data")


def _iter_data_files(directory):
    for fn in Path.iterdir(directory):
        if fn.suffix == ".svg":
            yield Path(directory).joinpath(fn)


def test_reading_scenes(gui_app):
    for fn in _iter_data_files(DATA_DIR):
        model = read_scene(fn)
        view = SceneView(model)
        view.show()
        assert view is not None
        view.destroy()
