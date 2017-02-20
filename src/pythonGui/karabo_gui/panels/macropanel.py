#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
""" This is the central panel to edit macros. """
from PyQt4.QtCore import Qt, QEvent
from PyQt4.QtGui import (QTextEdit, QPlainTextEdit, QMessageBox,
                         QSplitter, QTextCursor)

try:
    from qtconsole.pygments_highlighter import PygmentsHighlighter
except ImportError:
    from IPython.qt.console.pygments_highlighter import PygmentsHighlighter

from karabo.common.project.api import write_macro
from karabo_gui.events import (
    KaraboBroadcastEvent, KaraboEventSender, register_for_broadcasts,
    unregister_from_broadcasts)
import karabo_gui.icons as icons
from karabo_gui.project.utils import run_macro
from karabo_gui.singletons.api import get_topology
from karabo_gui.toolbar import ToolBar
from karabo_gui.util import getSaveFileName
from .base import BasePanelWidget


class MacroPanel(BasePanelWidget):
    def __init__(self, model):
        self.model = model

        super(MacroPanel, self).__init__(model.simple_name)

        self.already_connected = set()

        # Register to KaraboBroadcastEvent
        register_for_broadcasts(self)

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
        self.teEditor.textChanged.connect(self.onMacroChanged)

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
        toolbar.addAction(icons.start, "Run", self.onRun)
        toolbar.addAction(icons.save, "Save", self.onSave)
        return [toolbar]

    def eventFilter(self, object, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.teEditor.textCursor().insertText(" " * 4)
                return True
        elif isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            data = event.data
            if sender is KaraboEventSender.ConnectMacroInstance:
                model = data.get('model')
                if model is self.model:
                    self.connect(data.get('instance'))
            elif sender is KaraboEventSender.DeviceInitReply:
                macro_instance = data.get('device')
                if macro_instance.id in self.model.instance_id:
                    self.initReply(data.get('success'), data.get('message'))
            return False
        return False

    def closeEvent(self, event):
        # Unregister to KaraboBroadcastEvent
        unregister_from_broadcasts(self)
        event.accept()

    def connect(self, macro_instance):
        if macro_instance not in self.already_connected:
            device = get_topology().get_device(macro_instance)
            device.boxvalue.doNotCompressEvents.signalUpdateComponent.connect(
                self.appendConsole)
            self.already_connected.add(macro_instance)

    def appendConsole(self, box, value, ts):
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(box.configuration.value.print)

    def initReply(self, ok, error):
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(error)
        self.console.insertPlainText("\n")

    def onRun(self):
        self.console.clear()
        try:
            compile(self.model.code, self.model.simple_name, "exec")
        except SyntaxError as e:
            if e.filename[7:-3] == self.model.simple_name:
                c = self.teEditor.textCursor()
                c.movePosition(c.Start)
                c.movePosition(c.Down, n=e.lineno - 1)
                c.movePosition(c.Right, n=e.offset)
                self.teEditor.setTextCursor(c)
            QMessageBox.warning(self.teEditor, type(e).__name__,
                                "{}\n{}{}^\nin {} line {}".format(
                                e.msg, e.text, " " * e.offset, e.filename,
                                e.lineno))
        else:
            run_macro(self.model)

    def onSave(self):
        fn = getSaveFileName(
                caption="Save macro to file",
                filter="Python files (*.py)",
                suffix="py",
                selectFile=self.model.simple_name + ".py")
        if not fn:
            return

        with open(fn, "w") as out:
            out.write(write_macro(self.model))

    def onMacroChanged(self):
        self.model.code = self.teEditor.toPlainText()
