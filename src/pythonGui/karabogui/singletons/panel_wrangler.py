#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
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
#############################################################################
from qtpy.QtCore import QObject, Slot
from qtpy.QtWidgets import QPushButton

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui import icons
from karabogui.access import AccessRole, access_role_allowed, is_authenticated
from karabogui.controllers.util import load_extensions
from karabogui.dialogs.update_dialog import (
    UpdateNoticeDialog, is_package_updated)
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts)
from karabogui.indicators import get_user_session_button_data
from karabogui.logger import get_logger
from karabogui.mainwindow import CONFIGURATOR_TITLE, MainWindow, PanelAreaEnum
from karabogui.panels.api import (
    ConfigurationPanel, MacroPanel, ScenePanel, WidgetControllerPanel)
from karabogui.programs.utils import close_app
from karabogui.singletons.api import get_config, get_db_conn, get_project_model
from karabogui.util import (
    get_application_icon, move_to_cursor, process_qt_events, send_info)
from karabogui.wizards import TipsTricksWizard

GUI_EXTENSIONS = "GUIExtensions"


class PanelWrangler(QObject):
    """An object which handles wrangler duties for the myriad panels and
    windows shown in the GUI.

    Importantly, it also holds the reference to the main window.
    """

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        self.main_window = None
        self.splash = None
        self.connected_to_server = False
        self._editor = None
        # Panel containers
        # Project items (scenes, macros) {model: panel}
        self._project_item_panels = {}
        # Unattached scene panels {model: panel}
        self._unattached_scene_panels = {}

        # Unattached controller panels {`deviceId.path|Panel`: panel}
        self._widget_controller_panels = {}

        # Panels linked to instances {instance id: (panel, pos)}
        self._instance_panels = {}

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary due to the fact that the singleton mediator object and
        # `self` are being destroyed when the GUI exists
        event_map = {
            KaraboEvent.RestoreSceneView: self._event_restore_scene,
            KaraboEvent.ShowSceneView: self._event_attached_scene,
            KaraboEvent.ShowUnattachedSceneView: self._event_unattached_scene,
            KaraboEvent.ShowUnattachedController: self._event_controller_panel,
            KaraboEvent.OpenSceneLink: self._event_scene_link,
            KaraboEvent.RemoveProjectModelViews: self._event_remove_model_view,
            KaraboEvent.MiddlePanelClosed: self._event_middle_panel,
            KaraboEvent.ShowMacroView: self._event_show_macro,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.CreateMainWindow: self._event_mainwindow,
            KaraboEvent.RaiseEditor: self._event_raise_editor,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.UserSession: self._event_user_session,
        }
        register_for_broadcasts(event_map)

    # -------------------------------------------------------------------
    # public interface

    def toggleReloadExtensions(self):
        """Enable the reload extensions on the mainwindow"""
        button = QPushButton(icons.refresh, "Reload Extensions")
        button.clicked.connect(self._reload_extensions)
        button.setFixedHeight(40)
        button.setVisible(True)
        self.main_window.toolbar.addWidget(button)

    def is_showing_project_item(self, model):
        """Returns True if the given `model` has a view currently showing.
        """
        return model in self._project_item_panels

    def scene_panel_data(self):
        """Return the dictionary of opened project scene panels"""
        panel_data = self._project_item_panels
        return {model: panel for model, panel in panel_data.items()
                if isinstance(model, SceneModel)}

    def use_splash_screen(self, splash):
        """Attach a QSplashScreen instance which will be closed when the first
        window appears.
        """
        if self.main_window is not None:
            splash.finish(self.main_window)

        self.splash = splash

    # --------------------  -----------------------------------------------
    # Qt callbacks

    def _event_restore_scene(self, data):
        target_window = data.get("target_window",
                                 SceneTargetWindow.MainWindow)
        model = data["model"]
        panel = self._open_scene(model, target_window, attached=True)
        if panel is not None and not panel.is_docked:
            panel.move(data["x"], data["y"])

    def _event_attached_scene(self, data):
        target_window = data.get('target_window',
                                 SceneTargetWindow.MainWindow)
        model = data['model']
        self._open_scene(model, target_window, attached=True)

    def _event_controller_panel(self, data):
        model = data["model"]
        title = f"{model.keys[0]}|Panel"
        existing = self._widget_controller_panels.items()
        existing = [panel for key, panel in existing if title == key]

        # Create a new widget controller panels and afterwards close the old
        # ones. This way the visibility counter is not disturbed!
        panel = WidgetControllerPanel(title, model)
        panel.signalPanelClosed.connect(self._on_controller_panel_close)
        for old_panel in existing:
            self._close_panel(old_panel)

        self._widget_controller_panels[title] = panel
        main_win = self.main_window
        if main_win is None:
            panel.show()
            return

        main_win.addPanel(panel, PanelAreaEnum.Middle)
        panel.onUndock()

    def _event_unattached_scene(self, data):
        target_window = data.get('target_window',
                                 SceneTargetWindow.Dialog)
        model = data['model']

        # Unattached scenes are closed if they are duplicated by simple name!
        existing = [panel for scene, panel in
                    self._unattached_scene_panels.items()
                    if scene.simple_name == model.simple_name]
        scene_panel = self._open_scene(model, target_window, attached=False)
        for panel in existing:
            self._close_panel(panel)
        position = data.get("position")
        if position is not None and target_window == SceneTargetWindow.Dialog:
            scene_panel.move(*position)

    def _event_scene_link(self, data):
        name = data.get('name', '')
        target_window = data.get('target_window')
        uuid = data.get('target')
        model = _find_scene_model(name, uuid)
        if model is not None:
            # We found the scene in our project!
            self._open_scene(model, target_window)
        else:
            position = data.get("position")
            get_db_conn().get_database_scene(
                name, uuid, target_window, position=position)

    def _event_remove_model_view(self, data):
        self._close_project_item_panels(data['models'])

    def _event_middle_panel(self, data):
        model = data.get('model')
        if model in self._project_item_panels:
            self._project_item_panels.pop(model)
        elif model in self._unattached_scene_panels:
            self._unattached_scene_panels.pop(model)
        elif model in self._widget_controller_panels:
            self._widget_controller_panels.pop(model)

        # We don't have a main window, but are either running in cinema or
        # theatre. Hence, we can close the application if no panel has been
        # opened
        if self.main_window is None and not self._unattached_scene_panels:
            close_app()

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
            if self.main_window is not None:
                icon, tooltip = get_user_session_button_data()
                self.main_window.setSessionButton(icon, tooltip)

    def _event_mainwindow(self, data):
        self._create_main_window()

    def _event_raise_editor(self, data):
        self._raise_editor(data)

    def _event_access_level(self, data):
        """React on the access level changes and modify scene and macro panel
        """
        scene_editable = access_role_allowed(AccessRole.SCENE_EDIT)
        macro_editable = access_role_allowed(AccessRole.MACRO_EDIT)
        apply_scene_edit = access_role_allowed(AccessRole.APPLY_SCENE_EDIT)
        for panel in self._project_item_panels.values():
            if isinstance(panel, ScenePanel):
                panel.setReadOnly(scene_editable)
                panel.toggleAlwaysVisibleToolbar(apply_scene_edit)
            elif isinstance(panel, MacroPanel):
                panel.setReadOnly(not macro_editable)
        for panel in self._unattached_scene_panels.values():
            panel.toggleAlwaysVisibleToolbar(apply_scene_edit)

    def _event_user_session(self, data):
        """Update the User Session button in main window and all
        scene panels, when the Temporary session state changes"""
        icon, tooltip = get_user_session_button_data()
        if self.main_window is not None:
            self.main_window.setSessionButton(icon, tooltip)
        for panel in self._project_item_panels.values():
            if isinstance(panel, ScenePanel):
                panel.setSessionButton(icon, tooltip)
        for panel in self._unattached_scene_panels.values():
            panel.setSessionButton(icon, tooltip)

    # -------------------------------------------------------------------
    # private interface

    def _raise_editor(self, data):
        """Raise the main window or editor if existent and move to cursor"""
        if self.main_window is not None:
            self.main_window.dockPanel(CONFIGURATOR_TITLE)
            self.main_window.activateWindow()
            self.main_window.raise_()
            move_to_cursor(self.main_window)
        else:
            if self._editor is None:
                self._editor = ConfigurationPanel(allow_closing=True)
                self._editor.signalPanelClosed.connect(self._editor_closed)
                self._editor.toolbar.setVisible(True)
                self._editor.resize(600, 800)
                self._editor.show()
            self._editor.activateWindow()
            self._editor.raise_()
            move_to_cursor(self._editor)

        broadcast_event(KaraboEvent.ShowConfiguration, data)

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
    def _editor_closed(self, panel):
        """The dedicated editor singleton has been closed"""
        self._editor = None

    @Slot(str)
    def _on_tab_close(self, instance_id):
        """A panel emits its instance id when user close it, this function
        enables the action to reopen it
        """
        if instance_id in self._instance_panels:
            del self._instance_panels[instance_id]

    @Slot(str)
    def _on_controller_panel_close(self, panel):
        if panel in self._widget_controller_panels:
            del self._widget_controller_panels[panel]

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
        for panel in list(self._unattached_scene_panels.values()):
            self._close_panel(panel)
        for panel in list(self._widget_controller_panels.values()):
            self._close_panel(panel)

    def _create_main_window(self):
        if self.main_window is not None:
            return

        self.main_window = MainWindow()
        icon = get_application_icon()
        self.main_window.setWindowIcon(icon)
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

        check_updates = get_config()['check_updates']
        if check_updates and is_package_updated(GUI_EXTENSIONS):
            dialog = UpdateNoticeDialog(parent=self.main_window)
            dialog.exec()

        show_wizard = get_config()['wizard']
        if show_wizard:
            wizard = TipsTricksWizard(parent=self.main_window)
            # We can show the wizard if needed and block!
            wizard.exec()
            process_qt_events(timeout=1000)

        # Show the connection dialog after processing all pending events
        self.main_window.acServerConnect.trigger()
        if get_config()["development"]:
            self.toggleReloadExtensions()

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
        send_info(type="open_macro", uuid=model.uuid, name=model.simple_name)

    def _open_scene(self, model, target_window, attached=True):
        if model is None:
            print("Tried to open a scene which was None!")
            return
        panel = self._project_item_panels.get(model)
        if panel is None:
            model.assure_svg_data()

            panel = ScenePanel(model, self.connected_to_server)
            # Set the tool color according to the defined indicators!
            karabo_topic = get_config()['broker_topic']
            panel.set_toolbar_style(karabo_topic)
            enabled = is_authenticated()
            panel.setTemporaryButtonVisible(enabled)
            if enabled:
                # Sync the button state with current Temporary session state.
                icon, tooltip = get_user_session_button_data()
                panel.setSessionButton(icon, tooltip)

        # XXX: Only attached and access level dependent scene panels are
        # allowed to have design mode!
        editable = access_role_allowed(AccessRole.SCENE_EDIT)
        if not attached:
            panel.setUnattached(editable)
        else:
            panel.setReadOnly(editable)

        # The apply scene edit is global setting
        apply_scene_edit = access_role_allowed(AccessRole.APPLY_SCENE_EDIT)
        panel.toggleAlwaysVisibleToolbar(apply_scene_edit)

        self._show_project_item_panel(model, panel, attached)

        send_info(type="open_scene", uuid=model.uuid,
                  name=model.simple_name, attached=attached)
        if self.main_window is None:
            return panel

        if target_window is SceneTargetWindow.MainWindow:
            panel.onDock()
        elif target_window is SceneTargetWindow.Dialog:
            panel.onUndock()
        return panel

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

    @Slot()
    def _reload_extensions(self):
        """Request to reload the gui extensions and close all scenes"""
        get_logger().info("Reloading the GUI Extensions ...")
        load_extensions()
        panels = [p for p in self._project_item_panels.values()
                  if isinstance(p, ScenePanel)]
        for panel in panels:
            self._close_panel(panel)
        # And all unattached scene panels
        self._close_unattached_panels()


def _find_scene_model(name, uuid):
    """Find a SceneModel which is already open in the project.
    """

    class _Visitor:
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
               "project! Trying to retrieve the scene in from the project "
               "<b>database</b> instead.".format(name))
        get_logger().warning(msg)
        return None

    return visitor.found
