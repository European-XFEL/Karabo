#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QObject, Slot

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui import messagebox
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.globals import AccessRole, access_role_allowed
from karabogui.mainwindow import MainWindow, PanelAreaEnum
from karabogui.panels.api import MacroPanel, ScenePanel
from karabogui.singletons.api import get_config, get_db_conn, get_project_model
from karabogui.util import process_qt_events
from karabogui.wizards import TipsTricksWizard


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
        # Unattached scene panels {model: panel}
        self._unattached_scene_panels = {}

        # Panels linked to instances {instance id: (panel, pos)}
        self._instance_panels = {}

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary due to the fact that the singleton mediator object and
        # `self` are being destroyed when the GUI exists
        event_map = {
            KaraboEvent.ShowSceneView: self._event_attached_scene,
            KaraboEvent.ShowUnattachedSceneView: self._event_unattached_scene,
            KaraboEvent.OpenSceneLink: self._event_scene_link,
            KaraboEvent.RemoveProjectModelViews: self._event_remove_model_view,
            KaraboEvent.MiddlePanelClosed: self._event_middle_panel,
            KaraboEvent.ShowMacroView: self._event_show_macro,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.CreateMainWindow: self._event_mainwindow,
            KaraboEvent.AccessLevelChanged: self._event_access_level
        }
        register_for_broadcasts(event_map)

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

    # --------------------  -----------------------------------------------
    # Qt callbacks

    def _event_attached_scene(self, data):
        target_window = data.get('target_window',
                                 SceneTargetWindow.MainWindow)
        model = data['model']
        self._open_scene(model, target_window, attached=True)

    def _event_unattached_scene(self, data):
        target_window = data.get('target_window',
                                 SceneTargetWindow.MainWindow)
        model = data['model']

        # Unattached scenes are closed if they are duplicated by simple name!
        existing = [panel for scene, panel in
                    self._unattached_scene_panels.items()
                    if scene.simple_name == model.simple_name]

        self._open_scene(model, target_window, attached=False)
        for panel in existing:
            self._close_panel(panel)

    def _event_scene_link(self, data):
        name = data.get('name', '')
        target_window = data.get('target_window')
        uuid = data.get('target')
        model = _find_scene_model(name, uuid)
        if model is not None:
            # We found the scene in our project!
            self._open_scene(model, target_window)
        else:
            get_db_conn()._get_database_scene(name, uuid, target_window)

    def _event_remove_model_view(self, data):
        self._close_project_item_panels(data['models'])

    def _event_middle_panel(self, data):
        model = data.get('model')
        if model in self._project_item_panels:
            self._project_item_panels.pop(model)
        elif model in self._unattached_scene_panels:
            self._unattached_scene_panels.pop(model)

    def _event_show_macro(self, data):
        self._open_macro(data['model'])

    def _event_network(self, data):
        self.connected_to_server = data.get('status', False)
        if not self.connected_to_server:
            # Copy; _close_instance_panel will mutate
            panel_ids = list(self._instance_panels.keys())
            for inst_id in panel_ids:
                self._close_instance_panel(inst_id)
            # Close panels not associated with projects
            self._close_unattached_panels()

    def _event_mainwindow(self, data):
        self._create_main_window()

    def _event_access_level(self, data):
        """React on the access level changes and modify scene and macro panel
        """
        scene_editable = access_role_allowed(AccessRole.SCENE_EDIT)
        macro_editable = access_role_allowed(AccessRole.MACRO_EDIT)
        for panel in self._project_item_panels.values():
            if isinstance(panel, ScenePanel):
                panel.setReadOnly(scene_editable)
            elif isinstance(panel, MacroPanel):
                panel.setReadOnly(not macro_editable)

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

    @Slot(str)
    def _on_tab_close(self, instance_id):
        """A panel emits its instance id when user close it, this function
        enables the action to reopen it
        """
        if instance_id in self._instance_panels:
            del self._instance_panels[instance_id]

    def _close_panel(self, panel):
        if panel is None:
            return
        if self.main_window is None:
            panel.close()
        else:
            self.main_window.removePanel(panel, PanelAreaEnum.Middle)

    def _close_project_item_panels(self, models):
        for model in models:
            self._close_panel(self._project_item_panels.get(model))

    def _close_unattached_panels(self):
        for panel in self._unattached_scene_panels.values():
            self._close_panel(panel)

    def _create_main_window(self):
        if self.main_window is not None:
            return

        self.main_window = MainWindow()
        process_qt_events(timeout=1000)

        if self.splash is not None:
            self.splash.finish(self.main_window)
            # Finish splash
            process_qt_events(timeout=1000)

        # Show main window and process events!
        self.main_window.show()
        process_qt_events(timeout=1000)
        self.main_window.activateWindow()
        process_qt_events(timeout=1000)

        show_wizard = get_config()['wizard']
        if show_wizard:
            wizard = TipsTricksWizard(parent=self.main_window)
            # We can show the wizard if needed and block!
            wizard.exec()
            process_qt_events(timeout=1000)

        # Show the connection dialog after processing all pending events
        self.main_window.acServerConnect.trigger()

    def _open_macro(self, model):
        if model is None:
            print("Tried to open a macro which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = MacroPanel(model)

        editable = access_role_allowed(AccessRole.MACRO_EDIT)
        panel.ui_editor.setReadOnly(not editable)
        self._show_project_item_panel(model, panel)

    def _open_scene(self, model, target_window, attached=True):
        if model is None:
            print("Tried to open a scene which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            panel = ScenePanel(model, self.connected_to_server)
            # Set the tool color according to the defined indicators!
            karabo_topic = get_config()['broker_topic']
            panel.set_toolbar_style(karabo_topic)

        # XXX: Only attached and access level dependent scene panels are
        # allowed to have design mode!
        editable = access_role_allowed(AccessRole.SCENE_EDIT)
        panel.ac_design_mode.setVisible(attached and editable)
        self._show_project_item_panel(model, panel, attached)

        if self.main_window is None:
            return

        if target_window is SceneTargetWindow.MainWindow:
            panel.onDock()
        elif target_window is SceneTargetWindow.Dialog:
            panel.onUndock()

    def _show_project_item_panel(self, model, panel, attached=True):
        has_panel = model in self._project_item_panels

        if attached:
            self._project_item_panels[model] = panel
        else:
            self._unattached_scene_panels[model] = panel

        if self.splash is not None:
            self.splash.close()
            self.splash = None

        if self.main_window is None:
            panel.show()
            return
        if not has_panel:
            self.main_window.addPanel(panel, PanelAreaEnum.Middle)
        self.main_window.selectPanel(panel, PanelAreaEnum.Middle)


def _find_scene_model(name, uuid):
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
        msg = ("The UUID of the linked scene <b>{}</b> was not found in the "
               "project! Trying to retrieve the scene in <b>control mode</b> "
               "from the project <b>database</b> instead.".format(name))
        messagebox.show_error(msg)
        return None

    return visitor.found
