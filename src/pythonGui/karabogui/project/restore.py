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
from traits.api import Bool, Dict, HasStrictTraits, Instance, WeakRef

from karabo.common.api import walk_traits_object
from karabo.common.project.api import ProjectModel
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_panel_wrangler

from .loading_watcher import ProjectLoadingWatcher

_reload_sessions = set()


def add_restore(project, scene_data):
    """Add a session for bookkeeping"""
    global _reload_sessions
    _reload_sessions = {s for s in _reload_sessions if not s.finished}
    active = ProjectRestoreSession(project=project, scene_data=scene_data)
    _reload_sessions.add(active)


class ProjectRestoreSession(HasStrictTraits):
    """An object which can restore the last session
    """
    # A project to be restored
    project = WeakRef(ProjectModel)
    # A boolean that is set once the scene view events are broadcasted
    finished = Bool(False)
    # Dictionary for scene model meta data {uuid: Dict of data}
    scene_data = Dict
    # The watcher running in background
    _watcher = Instance(ProjectLoadingWatcher)

    def _project_changed(self):
        if self.project is not None:
            self._watcher = ProjectLoadingWatcher(project=self.project)
            self._watcher.on_trait_change(self._finished, "finished_event")

    def _finished(self):
        """Traits notification handler to broadcast actions
        """
        scene_view = []

        def visitor(model):
            if isinstance(model, SceneModel) and model.uuid in self.scene_data:
                data = {"model": model}
                data.update(self.scene_data.pop(model.uuid))
                scene_view.append(data)

        walk_traits_object(self.project, visitor)

        # Sort according to index
        scene_view = sorted(scene_view, key=lambda scene: scene["index"])
        for data in scene_view:
            broadcast_event(KaraboEvent.RestoreSceneView, data)
        self._watcher.on_trait_change(self._finished, "finished_event",
                                      remove=True)
        self.finished = True


def get_restore_data():
    """Retrieve existing meta data for scenes to restore"""
    wrangler = get_panel_wrangler()
    scene_data = wrangler.scene_panel_data()

    data = {}
    for model, panel in scene_data.items():
        target_window = (SceneTargetWindow.MainWindow if panel.is_docked
                         else SceneTargetWindow.Dialog)
        position = panel.pos()
        x, y = position.x(), position.y()
        data[model.uuid] = {
            "target_window": target_window,
            "x": x,
            "y": y,
            "index": panel.index,
        }
    return data
