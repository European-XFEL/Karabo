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
from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui.singletons.api import get_project_model

from .utils import get_dialog_ui


class SceneLinkDialog(QDialog):
    def __init__(self, model, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui('scenelink.ui'), self)

        self._selectedScene = 0
        self._sceneTargets = self._get_scenelink_targets()
        for scene in self._sceneTargets:
            self.lwScenes.addItem(scene)

        flags = Qt.MatchExactly | Qt.MatchCaseSensitive
        self._select_item(model.target, flags)

        self._selectedTargetWin = model.target_window
        radioButtons = {
            SceneTargetWindow.MainWindow: self.mainRadio,
            SceneTargetWindow.Dialog: self.dialogRadio
        }
        button = radioButtons.get(self._selectedTargetWin)
        if button:
            button.setChecked(True)

    def _get_scenelink_targets(self):
        project = get_project_model().root_model
        if project is None:
            return []

        collected = set()

        def visitor(obj):
            nonlocal collected
            if isinstance(obj, SceneModel):
                target = f"{obj.simple_name}:{obj.uuid}"
                collected.add(target)

        walk_traits_object(project, visitor)
        return sorted(collected)

    def _select_item(self, text, flags):
        items = self.lwScenes.findItems(text, flags)
        if items:
            # Select first result
            self.lwScenes.setCurrentItem(items[0])
        self._selectedScene = self.lwScenes.currentRow()

    @property
    def selectedScene(self):
        return self._sceneTargets[self._selectedScene]

    @property
    def selectedTargetWindow(self):
        return self._selectedTargetWin

    @Slot(int)
    def on_lwScenes_currentRowChanged(self, index):
        self._selectedScene = index

    @Slot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @Slot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow

    @Slot(str)
    def on_leFilter_textChanged(self, text):
        self._select_item(text, Qt.MatchStartsWith)
