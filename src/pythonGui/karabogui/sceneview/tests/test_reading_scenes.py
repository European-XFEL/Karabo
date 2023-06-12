# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
