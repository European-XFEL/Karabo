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

from karabo.middlelayer import Hash, write_macro
from karabo_gui.docktabwindow import Dockable
from karabo_gui.events import (
    KaraboBroadcastEvent, KaraboEventSender, register_for_broadcasts,
    unregister_from_broadcasts)
import karabo_gui.icons as icons
from karabo_gui.singletons.api import get_network
from karabo_gui.topology import getDevice
from karabo_gui.util import getSaveFileName


class MacroPanel(Dockable, QSplitter):

    def __init__(self, macro_model):
        QSplitter.__init__(self, Qt.Vertical)

        self.macro_model = macro_model

        self.teEditor = QTextEdit(self)
        self.teEditor.installEventFilter(self)
        self.teEditor.setAcceptRichText(False)
        self.teEditor.setStyleSheet("font-family: monospace")
        self.teEditor.setPlainText(write_macro(self.macro_model))

        PygmentsHighlighter(self.teEditor.document())
        self.addWidget(self.teEditor)
        self.teEditor.setLineWrapMode(QTextEdit.NoWrap)
        self.teEditor.textChanged.connect(self.onMacroChanged)

        self.console = QPlainTextEdit(self)
        self.console.setReadOnly(True)
        self.console.setStyleSheet("font-family: monospace")
        self.addWidget(self.console)
        self.already_connected = set()

        # Register to KaraboBroadcastEvent
        register_for_broadcasts(self)

        # Connect all running macros
        for instance in macro_model.instances:
            self.connect(instance)

    def setupToolBars(self, tb, parent):
        tb.addAction(icons.start, "Run", self.onRun)
        tb.addAction(icons.save, "Save", self.onSave)

    def eventFilter(self, object, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.teEditor.textCursor().insertText(" " * 4)
                return True
        elif isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            if sender is KaraboEventSender.ConnectMacroInstance:
                data = event.data
                macro_model = data.get('model')
                if macro_model is self.macro_model:
                    self.connect(data.get('instance'))
                    return False
            elif sender is KaraboEventSender.DeviceInitReply:
                data = event.data
                macro_instance = data.get('device')
                if macro_instance.id in self.macro_model.instance_id:
                    self.initReply(data.get('success'), data.get('message'))
                    return False
        return False

    def closeEvent(self, event):
        # Unregister to KaraboBroadcastEvent
        unregister_from_broadcasts(self)
        event.accept()

    def connect(self, macro_instance):
        if macro_instance not in self.already_connected:
            device = getDevice(macro_instance)
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
            compile(self.macro_model.code, self.macro_model.simple_name, "exec")
        except SyntaxError as e:
            if e.filename[7:-3] == self.macro_model.simple_name:
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
            instance_id = self.macro_model.instance_id
            getDevice(instance_id)
            h = Hash("code", self.macro_model.code,
                     "module", self.macro_model.simple_name,
                     "uuid", self.macro_model.uuid)
            get_network().onInitDevice("karabo/macroServer", "MetaMacro",
                                       instance_id, h)

    def onSave(self):
        fn = getSaveFileName(
                caption="Save macro to file",
                filter="Python files (*.py)",
                suffix="py",
                selectFile=self.macro_model.simple_name + ".py")
        if not fn:
            return

        with open(fn, "w") as out:
            out.write(write_macro(self.macro_model))

    def onMacroChanged(self):
        self.macro_model.code = self.teEditor.toPlainText()
