#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import QObject, pyqtSlot
from PyQt4.QtGui import QAction

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabo_gui.events import KaraboEventSender, register_for_broadcasts
from karabo_gui import icons
from karabo_gui.mainwindow import MainWindow, PanelAreaEnum
from karabo_gui import messagebox
from karabo_gui.panels.alarmpanel import AlarmPanel
from karabo_gui.panels.macropanel import MacroPanel
from karabo_gui.panels.runconfigpanel import RunConfigPanel
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.singletons.api import get_project_model

_ICONS = {
    RunConfigPanel: icons.runconfig,
}

# Map open instance panel event to the corresponding panel class
# and its position in the main window
_EVENT_PANEL_MAP = {
    KaraboEventSender.AddRunConfigurator:
        (RunConfigPanel, PanelAreaEnum.MiddleBottom),
}


class PanelWrangler(QObject):
    """An object which handles wrangler duties for the myriad panels and
    windows shown in the GUI.

    Importantly, it also holds the reference to the main window.
    """

    def __init__(self, parent=None):
        super(PanelWrangler, self).__init__(parent=parent)

        self.main_window = None
        self.splash = None
        self.connected_to_server = False

        # Panel containers
        # Project items (scenes, macros) {model: panel}
        self._project_item_panels = {}
        self._unattached_project_models = []

        # Panels linked to instances {instance id: (panel, pos)}
        self._instance_panels = {}

        # QActions for opening instance panels {instance id: (action, klass)}
        self._instance_panel_actions = {}

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary due to the fact that the singleton mediator object and
        # `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    # -------------------------------------------------------------------
    # public interface

    def is_showing_project_item(self, model):
        """Returns True if the given `model` has a view currently showing.
        """
        return model in self._project_item_panels

    def use_splash_screen(self, splash):
        """Attach a QSplashScreen instance which will be closed when the first
        window appears.
        """
        if self.main_window is not None:
            splash.finish(self.main_window)

        self.splash = splash

    # -------------------------------------------------------------------
    # Qt callbacks

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data

        if sender is KaraboEventSender.DeviceDataReceived:
            self._update_scenes()

        elif sender in (KaraboEventSender.ShowSceneView,
                        KaraboEventSender.ShowUnattachedSceneView):
            target_window = SceneTargetWindow.MainWindow
            model = data.get('model')
            attached = sender is KaraboEventSender.ShowSceneView
            self._open_scene(model, target_window, attached=attached)

        elif sender is KaraboEventSender.OpenSceneLink:
            target_window = data.get('target_window')
            model = _find_scene_model(data.get('target'))
            self._open_scene(model, target_window)

        elif sender is KaraboEventSender.RemoveProjectModelViews:
            self._close_project_item_panels(data.get('models'))

        elif sender is KaraboEventSender.MiddlePanelClosed:
            model = data.get('model')
            if model in self._project_item_panels:
                self._project_item_panels.pop(model)
            elif model in self._unattached_project_models:
                self._unattached_project_models.remove(model)

        elif sender is KaraboEventSender.ShowMacroView:
            self._open_macro(data.get('model'))

        elif sender is KaraboEventSender.ShowAlarmServices:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._open_instance_panel(inst_id, AlarmPanel,
                                          PanelAreaEnum.MiddleBottom)

        elif sender in _EVENT_PANEL_MAP:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._request_instance_panel(inst_id, sender)

        elif sender in (KaraboEventSender.RemoveAlarmServices,
                        KaraboEventSender.RemoveRunConfigurator):
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._close_instance_panel(inst_id)

        elif sender is KaraboEventSender.NetworkConnectStatus:
            self.connected_to_server = data.get('status', False)
            if not self.connected_to_server:
                inst_list = set(self._instance_panels.keys()).union(
                                    self._instance_panel_actions.keys())
                for inst_id in inst_list:
                    self._close_instance_panel(inst_id)
                # Close panels not associated with projects
                self._close_project_item_panels(
                    self._unattached_project_models)

        elif sender is KaraboEventSender.CreateMainWindow:
            self._create_main_window()

        return False

    # -------------------------------------------------------------------
    # private interface

    def _request_instance_panel(self, instance_id, sender):
        """called when karabo broadcast_event requires a new panel to be opened
        Depends on the panel's class, this function opens a panel or create
        a QAction in the view menu.
        """
        main_win = self.main_window
        if main_win is None:
            return
        known_ids = set(self._instance_panels.keys()).union(
                            self._instance_panel_actions.keys())
        if instance_id in known_ids:
            return
        panel_info = _EVENT_PANEL_MAP.get(sender)
        if panel_info is None:
            return
        klass, area_enum = panel_info
        klass_name = klass.__name__

        # only create QAction to open the panel
        action = QAction(instance_id, main_win)
        callback = partial(self._open_instance_panel,
                           instance_id, klass, area_enum)
        action.triggered.connect(callback)
        self._instance_panel_actions[instance_id] = (action, klass_name)
        icon = _ICONS.get(klass)
        main_win.addViewMenuAction(action, klass_name, icon)

    def _open_instance_panel(self, instance_id, klass, area_enum):
        """Add a panel to main window, hide the QAction button if necessary
        """
        if instance_id in self._instance_panels:
            return
        main_win = self.main_window
        panel = klass(instance_id)
        panel.signalPanelClosed.connect(self._on_tab_close)
        main_win.addPanel(panel, area_enum)
        self._instance_panels[instance_id] = (panel, area_enum)
        action_info = self._instance_panel_actions.get(instance_id)
        if action_info is not None:
            action, _ = action_info
            action.setVisible(False)
            main_win.updateViewMenu()

    def _close_instance_panel(self, instance_id):
        """Remove the panel and its linked QAction from the main window
        """
        main_win = self.main_window
        if main_win is None:
            return
        if instance_id in self._instance_panels:
            panel, area_enum = self._instance_panels.pop(instance_id)
            main_win.removePanel(panel, area_enum)
        if instance_id in self._instance_panel_actions:
            action, name = self._instance_panel_actions.pop(instance_id)
            main_win.removeViewMenuAction(action, name)

    @pyqtSlot(str)
    def _on_tab_close(self, instance_id):
        """A panel emits its instance id when user close it, this function
        enables the action to reopen it
        """
        action_info = self._instance_panel_actions.get(instance_id)
        if action_info is not None:
            action, _ = action_info
            action.setVisible(True)
            main_win = self.main_window
            if main_win is not None:
                main_win.updateViewMenu()
        if instance_id in self._instance_panels:
            del self._instance_panels[instance_id]

    def _close_project_item_panels(self, models):
        for model in models:
            panel = self._project_item_panels.get(model)
            if panel is None:
                continue
            if self.main_window is None:
                panel.close()
            else:
                self.main_window.removePanel(panel, PanelAreaEnum.MiddleTop)

    def _create_main_window(self):
        if self.main_window is not None:
            return

        self.main_window = MainWindow()
        if self.splash is not None:
            self.splash.finish(self.main_window)

        self.main_window.show()

    def _open_macro(self, model):
        if model is None:
            print("Tried to open a macro which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = MacroPanel(model)
        self._show_project_item_panel(model, panel)

    def _open_scene(self, model, target_window, attached=True):
        if model is None:
            print("Tried to open a scene which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = ScenePanel(model, self.connected_to_server)
        self._show_project_item_panel(model, panel)

        if not attached:
            self._unattached_project_models.append(model)

        if self.main_window is None:
            return

        if target_window is SceneTargetWindow.MainWindow:
            panel.onDock()
        elif target_window is SceneTargetWindow.Dialog:
            panel.onUndock()

    def _show_project_item_panel(self, model, panel):
        has_panel = model in self._project_item_panels
        self._project_item_panels[model] = panel

        if self.splash is not None:
            self.splash.close()
            self.splash = None

        if self.main_window is None:
            panel.show()
            return
        if not has_panel:
            self.main_window.addPanel(panel, PanelAreaEnum.MiddleTop)
        self.main_window.selectPanel(panel, PanelAreaEnum.MiddleTop)

    def _update_scenes(self):
        for model, panel in self._project_item_panels.items():
            if not isinstance(model, SceneModel):
                continue

            scene_view = panel.scene_view
            if scene_view is not None and scene_view.isVisible():
                scene_view.update()


def _find_scene_model(uuid):
    """Find a SceneModel which is already open in the project.
    """
    class _Visitor(object):
        found = None

        def __call__(self, obj):
            if isinstance(obj, SceneModel):
                if obj.uuid == uuid:
                    self.found = obj
                    return True

    project = get_project_model().traits_data_model
    if project is None:
        return None

    visitor = _Visitor()
    walk_traits_object(project, visitor, fast_exit=True)

    if visitor.found is None:
        msg = 'Linked scene with UUID "{}" not found!'.format(uuid)
        messagebox.show_error(msg)
        return None

    return visitor.found
