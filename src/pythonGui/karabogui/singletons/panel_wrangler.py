#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QObject, pyqtSlot

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui import messagebox
from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.mainwindow import MainWindow, PanelAreaEnum
from karabogui.panels.api import AlarmPanel, MacroPanel, ScenePanel
from karabogui.singletons.api import get_project_model


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
            target_window = data.get(
                'target_window', SceneTargetWindow.MainWindow)
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

        elif sender is KaraboEventSender.RemoveAlarmServices:
            instance_ids = data.get('instanceIds')
            for inst_id in instance_ids:
                self._close_instance_panel(inst_id)

        elif sender is KaraboEventSender.NetworkConnectStatus:
            self.connected_to_server = data.get('status', False)
            if not self.connected_to_server:
                # Copy; _close_instance_panel will mutate
                panel_ids = list(self._instance_panels.keys())
                for inst_id in panel_ids:
                    self._close_instance_panel(inst_id)
                # Close panels not associated with projects
                self._close_project_item_panels(
                    self._unattached_project_models)

        elif sender is KaraboEventSender.CreateMainWindow:
            self._create_main_window()
            # we are the only one interested!
            return True

        return False

    # -------------------------------------------------------------------
    # private interface

    def _open_instance_panel(self, instance_id, klass, area_enum):
        """Add a panel to main window, hide the QAction button if necessary
        """
        if instance_id in self._instance_panels:
            return
        main_win = self.main_window
        if main_win is None:
            return
        panel = klass(instance_id)
        panel.signalPanelClosed.connect(self._on_tab_close)
        main_win.addPanel(panel, area_enum)
        self._instance_panels[instance_id] = (panel, area_enum)

    def _close_instance_panel(self, instance_id):
        """Remove the panel and its linked QAction from the main window
        """
        main_win = self.main_window
        if main_win is None:
            return
        if instance_id in self._instance_panels:
            panel, area_enum = self._instance_panels.pop(instance_id)
            main_win.removePanel(panel, area_enum)

    @pyqtSlot(str)
    def _on_tab_close(self, instance_id):
        """A panel emits its instance id when user close it, this function
        enables the action to reopen it
        """
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
        # NOTE: Only attached Scene panels are allowed to have design mode!
        panel.ac_design_mode.setVisible(attached)
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

    project = get_project_model().root_model
    if project is None:
        return None

    visitor = _Visitor()
    walk_traits_object(project, visitor, fast_exit=True)

    if visitor.found is None:
        msg = 'Linked scene with UUID "{}" not found!'.format(uuid)
        messagebox.show_error(msg)
        return None

    return visitor.found
