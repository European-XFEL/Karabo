#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt, QEvent
from PyQt4.QtGui import QTextEdit, QPlainTextEdit, QSplitter, QTextCursor

try:
    from qtconsole.pygments_highlighter import PygmentsHighlighter
except ImportError:
    from IPython.qt.console.pygments_highlighter import PygmentsHighlighter

from karabo.common.project.api import write_macro
from karabogui.events import (
    KaraboEventSender, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui import icons, messagebox
from karabogui.binding.api import PropertyProxy
from karabogui.project.utils import run_macro
from karabogui.singletons.api import get_topology
from karabogui.widgets.toolbar import ToolBar
from karabogui.util import getSaveFileName
from .base import BasePanelWidget


class MacroPanel(BasePanelWidget):
    def __init__(self, model):
        self.model = model
        super(MacroPanel, self).__init__(model.simple_name, allow_closing=True)

        self.already_connected = {}

        # Register to KaraboBroadcastEvent
        register_for_broadcasts(self)

        # Hook up a trait handler for the panel window title
        self.model.on_trait_change(self.set_title, 'simple_name')
        # Connect all running macros
        for instance in model.instances:
            self.connect(instance)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QSplitter(Qt.Vertical, parent=self)
        self.teEditor = QTextEdit(widget)
        self.teEditor.installEventFilter(self)
        self.teEditor.setAcceptRichText(False)
        self.teEditor.setStyleSheet("font-family: monospace")
        self.teEditor.setPlainText(write_macro(self.model))

        PygmentsHighlighter(self.teEditor.document())
        widget.addWidget(self.teEditor)
        self.teEditor.setLineWrapMode(QTextEdit.NoWrap)
        self.teEditor.textChanged.connect(self.on_macro_changed)

        self.console = QPlainTextEdit(widget)
        self.console.setReadOnly(True)
        self.console.setStyleSheet("font-family: monospace")
        widget.addWidget(self.console)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        toolbar = ToolBar(parent=self)
        toolbar.addAction(icons.start, "Run", self.on_run)
        toolbar.addAction(icons.save, "Save", self.on_save)
        return [toolbar]

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.teEditor.textCursor().insertText(" " * 4)
                return True
        return False

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.ConnectMacroInstance:
            model = data.get('model')
            if model is self.model:
                self.connect(data.get('instance'))
        elif sender is KaraboEventSender.DeviceInitReply:
            macro_instance = data.get('device')
            if macro_instance.device_id in self.model.instance_id:
                self.init_reply(data.get('success'), data.get('message'))
        return False

    def closeEvent(self, event):
        super(MacroPanel, self).closeEvent(event)
        if event.isAccepted():
            # Unregister to KaraboBroadcastEvent
            unregister_from_broadcasts(self)
            # Unregister the trait handler too
            self.model.on_trait_change(self.set_title, 'simple_name',
                                       remove=True)
            # Tell the world we're closing
            broadcast_event(KaraboEventSender.MiddlePanelClosed,
                            {'model': self.model})
            # Unregister all trait handlers
            for instance in self.model.instances:
                self.disconnect(instance)

    def connect(self, macro_instance):
        if macro_instance not in self.already_connected:
            macro_proxy = get_topology().get_device(macro_instance)
            self.console.moveCursor(QTextCursor.End)
            self.console.insertPlainText('connecting to {} '
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
            self.console.moveCursor(QTextCursor.End)
            self.console.insertPlainText(' connection done\n')
            self.already_connected[macro_proxy.device_id] = text_proxy
        macro_proxy.on_trait_change(self.finish_connection,
                                    'schema_update', remove=True)

    def append_console(self, binding, name, new):
        if name != 'config_update':
            return
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(binding.value)

    def init_reply(self, ok, message):
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(message)
        self.console.insertPlainText("\n")

    @pyqtSlot()
    def on_run(self):
        self.console.clear()
        try:
            compile(self.model.code, self.model.simple_name, "exec")
        except SyntaxError as e:
            if e.filename == self.model.simple_name:
                c = self.teEditor.textCursor()
                c.movePosition(c.Start)
                c.movePosition(c.Down, n=e.lineno - 1)
                c.movePosition(c.Right, n=e.offset)
                self.teEditor.setTextCursor(c)
            formatted_msg = "{}\n{}{}^\nin {} line {}".format(
                    e.msg, e.text, " " * e.offset, e.filename, e.lineno)
            messagebox.show_warning(formatted_msg, title=type(e).__name__)
        else:
            run_macro(self.model)

    @pyqtSlot()
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

    @pyqtSlot()
    def on_macro_changed(self):
        self.model.code = self.teEditor.toPlainText()
