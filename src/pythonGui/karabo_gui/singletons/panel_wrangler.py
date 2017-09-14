#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QObject

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabo_gui.events import KaraboEventSender, register_for_broadcasts
from karabo_gui.mainwindow import MainWindow, PanelAreaEnum
from karabo_gui import messagebox
from karabo_gui.panels.alarmpanel import AlarmPanel
from karabo_gui.panels.macropanel import MacroPanel
from karabo_gui.panels.runconfigpanel import RunConfigPanel
from karabo_gui.panels.runconfiggrouppanel import RunConfigGroupPanel
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.singletons.api import get_project_model


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
        # Alarm panels {instance id: panel}
        self._alarm_panels = {}
        # Run Configuration panels {instance id: panel}
        self._run_config_panels = {}
        # Run Configuration Group panels {instance id: panel}
        self._run_config_group_panels = {}

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

        elif sender is KaraboEventSender.ShowSceneView:
            target_window = SceneTargetWindow.MainWindow
            model = data.get('model')
            self._open_scene(model, target_window)

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

        elif sender is KaraboEventSender.ShowMacroView:
            self._open_macro(data.get('model'))

        elif sender is KaraboEventSender.ShowAlarmServices:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._open_alarm_panel(inst_id)

        elif sender is KaraboEventSender.AddRunConfigurator:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._open_run_configurator(inst_id)

        elif sender is KaraboEventSender.AddRunConfigGroup:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._open_run_config_group(inst_id)

        elif sender is KaraboEventSender.RemoveAlarmServices:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._close_alarm_panel(inst_id)

        elif sender is KaraboEventSender.RemoveRunConfigurator:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._close_run_configurator(inst_id)

        elif sender is KaraboEventSender.RemoveRunConfigGroup:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._close_run_config_group(inst_id)

        elif sender is KaraboEventSender.NetworkConnectStatus:
            self.connected_to_server = data.get('status', False)
            if not self.connected_to_server:
                # Close alarm panels
                alarm_ids = list(self._alarm_panels.keys())
                for inst_id in alarm_ids:
                    self._close_alarm_panel(inst_id)
                # Close run configuration panels
                instance_ids = list(self._run_config_panels.keys())
                for inst_id in instance_ids:
                    self._close_run_configurator(inst_id)
                # Close run configuration group panels
                instance_ids = list(self._run_config_group_panels.keys())
                for inst_id in instance_ids:
                    self._close_run_config_group(inst_id)

        elif sender is KaraboEventSender.CreateMainWindow:
            self._create_main_window()

        return False

    # -------------------------------------------------------------------
    # private interface

    def _close_alarm_panel(self, instance_id):
        panel = self._alarm_panels.get(instance_id)
        if panel is None:
            return
        del self._alarm_panels[instance_id]
        if self.main_window is not None:
            self.main_window.removePanel(panel, PanelAreaEnum.MiddleBottom)

    def _close_run_configurator(self, instance_id):
        panel = self._run_config_panels.get(instance_id)
        if panel is None:
            return
        del self._run_config_panels[instance_id]
        if self.main_window is not None:
            self.main_window.removePanel(panel, PanelAreaEnum.Right)

    def _close_run_config_group(self, instance_id):
        panel = self._run_config_group_panels.get(instance_id)
        if panel is None:
            return
        del self._run_config_group_panels[instance_id]
        if self.main_window is not None:
            self.main_window.removePanel(panel, PanelAreaEnum.Right)

    def _close_project_item_panels(self, models):
        for model in models:
            panel = self._project_item_panels.get(model)
            if panel is None:
                continue
            if self.main_window is None:
                panel.force_close()
            else:
                self.main_window.removePanel(panel, PanelAreaEnum.MiddleTop)

    def _create_main_window(self):
        if self.main_window is not None:
            return

        self.main_window = MainWindow()
        if self.splash is not None:
            self.splash.finish(self.main_window)

        self.main_window.show()

    def _open_alarm_panel(self, instance_id):
        if self.main_window is None:
            return

        main_win = self.main_window
        panel = self._alarm_panels.get(instance_id)
        if panel is None:
            title = "Alarms for {}".format(instance_id)
            panel = AlarmPanel(instance_id, title)
            main_win.addPanel(panel, PanelAreaEnum.MiddleBottom)
            self._alarm_panels[instance_id] = panel

    def _open_run_configurator(self, instance_id):
        if self.main_window is None:
            return

        main_win = self.main_window
        panel = self._run_config_panels.get(instance_id)
        if panel is None:
            title = "Run configuration at {}".format(instance_id)
            if len(self._run_config_panels) == 0:
                title = "RunConfig"
            panel = RunConfigPanel(instance_id, title)
            main_win.addPanel(panel, PanelAreaEnum.Right)
            self._run_config_panels[instance_id] = panel

    def _open_run_config_group(self, instance_id):
        if self.main_window is None:
            return

        main_win = self.main_window
        panel = self._run_config_group_panels.get(instance_id)
        if panel is None:
            title = instance_id
            panel = RunConfigGroupPanel(instance_id, title)
            main_win.addPanel(panel, PanelAreaEnum.Right)
            self._run_config_group_panels[instance_id] = panel

    def _open_macro(self, model):
        if model is None:
            print("Tried to open a macro which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = MacroPanel(model)
        self._show_project_item_panel(model, panel)

    def _open_scene(self, model, target_window):
        if model is None:
            print("Tried to open a scene which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = ScenePanel(model, self.connected_to_server)
        self._show_project_item_panel(model, panel)

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

    project = get_project_model().traits_data_model
    if project is None:
        return None

    visitor = _Visitor()
    walk_traits_object(project, visitor)

    if visitor.found is None:
        msg = 'Linked scene with UUID "{}" not found!'.format(uuid)
        messagebox.show_error(msg)
        return None

    return visitor.found
