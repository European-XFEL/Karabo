#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QObject

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts)
from karabo_gui.mainwindow import MainWindow
from karabo_gui.singletons.api import get_project_model


class PanelWrangler(QObject):
    """An object which handles wrangler duties for the myriad panels and
    windows shown in the GUI.

    Importantly, it also holds the reference to the main window.
    """

    def __init__(self, parent=None):
        super(PanelWrangler, self).__init__(parent=parent)

        # XXX: Currently `karabo_gui.gui` is reponsible for showing our window!
        self.main_window = MainWindow()

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary due to the fact that the singleton mediator object and
        # `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            data = event.data
            main_win = self.main_window
            if sender is KaraboEventSender.OpenSceneView:
                main_win.addSceneView(data.get('model'),
                                      SceneTargetWindow.MainWindow)
            elif sender is KaraboEventSender.OpenSceneLink:
                target = data.get('target')
                target_window = data.get('target_window')
                main_win.addSceneView(_find_scene_model(target),
                                      target_window)
            elif sender is KaraboEventSender.RemoveSceneView:
                main_win.removeMiddlePanel('scene_model', data.get('model'))
            elif sender is KaraboEventSender.RenameSceneView:
                main_win.renameMiddlePanel('scene_model', data.get('model'))
            elif sender is KaraboEventSender.OpenMacro:
                main_win.addMacro(data.get('model'))
                # XXX: I have no idea why, but this event causes an infinite
                # loop if it's not 'handled' here.
                return True
            elif sender is KaraboEventSender.RemoveMacro:
                main_win.removeMiddlePanel('macro_model', data.get('model'))
            elif sender is KaraboEventSender.RenameMacro:
                main_win.renameMiddlePanel('macro_model', data.get('model'))
            elif sender is KaraboEventSender.ShowAlarmServices:
                main_win.showAlarmServicePanels(data.get('instanceIds'))
            elif sender in (KaraboEventSender.AlarmInitReply,
                            KaraboEventSender.AlarmUpdate):
                main_win.showAlarmServicePanels([data.get('instanceId')])
            elif sender is KaraboEventSender.RemoveAlarmServices:
                main_win.removeAlarmServicePanels(data.get('instanceIds'))
            elif sender is KaraboEventSender.AddRunConfigurator:
                main_win.addRunConfigPanel(data.get('instanceIds'))
            elif sender is KaraboEventSender.RemoveRunConfigurator:
                main_win.removeRunConfigPanels(data.get('instanceIds'))
            return False
        return super(PanelWrangler, self).eventFilter(obj, event)


def _find_scene_model(uuid):
    """Find a SceneModel which is already open in the project.
    """
    class _Visitor(object):
        found = None

        def __call__(self, obj):
            if isinstance(obj, SceneModel):
                if obj.uuid == uuid:
                    self.found = obj

    project = get_project_model().traits_data_model
    if project is None:
        return None

    visitor = _Visitor()
    walk_traits_object(project, visitor)
    return visitor.found
