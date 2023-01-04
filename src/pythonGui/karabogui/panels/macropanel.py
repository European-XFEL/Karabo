#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QPoint, Qt, Slot
from qtpy.QtGui import QTextCursor
from qtpy.QtWidgets import QMenu, QPlainTextEdit, QSplitter

import karabogui.access as krb_access
from karabo.common.project.api import write_macro
from karabogui import icons, messagebox
from karabogui.binding.api import PropertyProxy
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.project.utils import run_macro
from karabogui.singletons.api import get_topology
from karabogui.util import getSaveFileName
# from karabogui.widgets.codeeditor import CodeBook
from karabogui.widgets.scintilla_editor import CodeBook
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget


class MacroPanel(BasePanelWidget):

    def __init__(self, model):
        self.model = model
        super(MacroPanel, self).__init__(model.simple_name, allow_closing=True)

        self.already_connected = {}

        # Register to KaraboBroadcastEvent
        self.event_map = {
            KaraboEvent.ConnectMacroInstance: self._event_connect,
            KaraboEvent.DeviceInitReply: self._event_init_reply,
        }
        register_for_broadcasts(self.event_map)

        # Hook up a trait handler for the panel window title
        self.model.on_trait_change(self.set_title, 'simple_name')
        # Connect all running macros
        for instance in model.instances:
            self.connect(instance)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QSplitter(Qt.Vertical, parent=self)
        ui_editor = CodeBook(parent=widget, code=write_macro(self.model))
        widget.addWidget(ui_editor)
        ui_editor.codeChanged.connect(self.on_macro_changed)
        self.ui_editor = ui_editor

        self.ui_console = QPlainTextEdit(widget)
        self.ui_console.setReadOnly(True)
        self.ui_console.setStyleSheet("font-family: monospace")
        self.ui_console.setContextMenuPolicy(Qt.CustomContextMenu)
        self.ui_console.customContextMenuRequested.connect(
            self._show_context_menu)

        widget.addWidget(self.ui_console)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        toolbar = ToolBar(parent=self)
        toolbar.addAction(icons.run, "Run", self.on_run)
        toolbar.addAction(icons.save, "Save", self.on_save)
        toolbar.addSeparator()
        toolbar.addAction(icons.zoomIn, "Increase font", self.increaseFont)
        toolbar.addAction(icons.zoomOut, "Decrease font", self.decreaseFont)
        return [toolbar]

    @Slot()
    def increaseFont(self):
        self.ui_editor.increaseFontSize()

    @Slot()
    def decreaseFont(self):
        self.ui_editor.decreaseFontSize()

    # -----------------------------------------------------------------------

    def setReadOnly(self, value):
        """This method is externally called for `AccessLevel` dependent view"""
        self.ui_editor.setReadOnly(value)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_connect(self, data):
        model = data.get('model')
        if model is self.model:
            self.connect(data.get('instance'))

    def _event_init_reply(self, data):
        macro_instance = data.get('device')
        if macro_instance.device_id in self.model.instance_id:
            self.init_reply(data.get('success'), data.get('message'))

    # -----------------------------------------------------------------------

    def closeEvent(self, event):
        super(MacroPanel, self).closeEvent(event)
        if event.isAccepted():
            # Unregister to KaraboBroadcastEvent
            unregister_from_broadcasts(self.event_map)
            # Unregister the trait handler too
            self.model.on_trait_change(self.set_title, 'simple_name',
                                       remove=True)
            # Tell the world we're closing
            broadcast_event(KaraboEvent.MiddlePanelClosed,
                            {'model': self.model})
            # Unregister all trait handlers
            for instance in self.model.instances:
                self.disconnect(instance)

    def connect(self, macro_instance):
        if macro_instance not in self.already_connected:
            macro_proxy = get_topology().get_device(macro_instance)
            self.ui_console.moveCursor(QTextCursor.End)
            self.ui_console.insertPlainText('Connecting to {} '
                                            '...'.format(macro_instance))
            macro_proxy.on_trait_change(self.finish_connection,
                                        'schema_update')
            if len(macro_proxy.binding.value) != 0:
                # if the proxy is already populated trigger the action
                self.finish_connection(macro_proxy,
                                       name='schema_update', new=None)

    def disconnect(self, macro_instance):
        if macro_instance in self.already_connected:
            text_proxy = self.already_connected.pop(macro_instance)
            text_proxy.stop_monitoring()
            text_proxy.on_trait_change(self.append_console,
                                       'binding.config_update', remove=True)

    def finish_connection(self, macro_proxy, name, new):
        if macro_proxy.device_id not in self.already_connected:
            text_proxy = PropertyProxy(root_proxy=macro_proxy, path='print')
            text_proxy.on_trait_change(
                self.append_console, 'binding.config_update')
            # start monitoring property
            text_proxy.start_monitoring()
            self.ui_console.moveCursor(QTextCursor.End)
            self.ui_console.insertPlainText('Connection done!\n')
            self.already_connected[macro_proxy.device_id] = text_proxy
        macro_proxy.on_trait_change(self.finish_connection,
                                    'schema_update', remove=True)

    def append_console(self, binding, name, new):
        if name != 'config_update':
            return
        self.ui_console.moveCursor(QTextCursor.End)
        self.ui_console.insertPlainText(binding.value)

    def init_reply(self, ok, message):
        self.ui_console.moveCursor(QTextCursor.End)
        self.ui_console.insertPlainText(message)
        self.ui_console.insertPlainText("\n")
        self.ui_console.moveCursor(QTextCursor.End)

    @Slot(QPoint)
    def _show_context_menu(self, pos):
        """Show a context menu"""
        menu = QMenu()
        clear_action = menu.addAction('Clear Console')
        clear_action.triggered.connect(self.ui_console.clear)
        select_action = menu.addAction('Select All')
        select_action.triggered.connect(self.ui_console.selectAll)
        menu.exec(self.ui_console.viewport().mapToGlobal(pos))

    @Slot()
    def on_run(self):
        allowed = krb_access.access_role_allowed(
            krb_access.AccessRole.SERVICE_EDIT)
        if not allowed:
            msg = (f"The current access level "
                   f"'{krb_access.GLOBAL_ACCESS_LEVEL}' "
                   f"is not sufficient to run the macro! Please contact a "
                   f"controls expert.")
            messagebox.show_information(msg)
            return

        self.ui_console.clear()
        try:
            compile(self.model.code, self.model.simple_name, "exec")
        except SyntaxError as e:
            if e.filename == self.model.simple_name:
                self.ui_editor.moveCursorToLine(e.lineno, e.offset)
            formatted_msg = "{}\n{}{}^\nin {} line {}".format(
                    e.msg, e.text, " " * e.offset, e.filename, e.lineno)
            messagebox.show_warning(formatted_msg, title=type(e).__name__)
            return

        run_macro(self.model)

    @Slot()
    def on_save(self):
        fn = getSaveFileName(
                caption="Save macro to file",
                filter="Python files (*.py)",
                suffix="py",
                selectFile=self.model.simple_name + ".py")
        if not fn:
            return

        with open(fn, "w") as out:
            out.write(write_macro(self.model))

    @Slot()
    def on_macro_changed(self):
        self.model.code = self.ui_editor.getEditorCode()
