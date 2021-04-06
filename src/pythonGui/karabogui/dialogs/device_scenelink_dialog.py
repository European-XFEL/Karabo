from collections import namedtuple

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import SceneTargetWindow

from .utils import get_dialog_ui

LinkModel = namedtuple('LinkModel', ['target', 'target_window'])


class DeviceSceneLinkDialog(QDialog):
    def __init__(self, scene_list, model, parent=None):
        """A dialog to select the device scene link

        :param scene_list: The list of the scenes
        :param model: The DeviceSceneLink model
        :param parent: The parent of the dialog
        """
        super(DeviceSceneLinkDialog, self).__init__(parent)
        filepath = get_dialog_ui('device_scenelink.ui')
        uic.loadUi(filepath, self)
        for scene in scene_list:
            self.cbScenes.addItem(scene)

        if model.target != '':
            index = self.cbScenes.findText(model.target)
            self.cbScenes.setCurrentIndex(index)

        radioButtons = {
            SceneTargetWindow.MainWindow: self.mainRadio,
            SceneTargetWindow.Dialog: self.dialogRadio
        }
        self._selectedTargetWin = model.target_window
        button = radioButtons.get(self._selectedTargetWin)
        if button:
            button.setChecked(True)

    @property
    def link_model(self):
        return LinkModel(target=self.cbScenes.currentText(),
                         target_window=self._selectedTargetWin)

    @Slot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @Slot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow
