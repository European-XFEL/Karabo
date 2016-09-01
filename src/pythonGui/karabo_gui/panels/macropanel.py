#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
""" This is the central panel to edit macros. """
from PyQt4.QtCore import Qt, pyqtSignal, QEvent
from PyQt4.QtGui import (QTextEdit, QPlainTextEdit, QMessageBox,
                         QSplitter, QTextCursor)
from qtconsole.pygments_highlighter import PygmentsHighlighter

from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.topology import getDevice
from karabo_gui.util import getSaveFileName


class MacroPanel(Dockable, QSplitter):

    def __init__(self, macroModel):
        QSplitter.__init__(self, Qt.Vertical)

        self.teEditor = QTextEdit(self)
        self.teEditor.installEventFilter(self)
        self.teEditor.setAcceptRichText(False)
        self.teEditor.setStyleSheet("font-family: monospace")
        try:
            self.teEditor.setPlainText(macroModel.code)
        except KeyError:
            pass
        PygmentsHighlighter(self.teEditor.document())
        self.addWidget(self.teEditor)
        self.teEditor.setLineWrapMode(QTextEdit.NoWrap)
        self.teEditor.textChanged.connect(self.onMacroChanged)

        self.console = QPlainTextEdit(self)
        self.console.setReadOnly(True)
        self.console.setStyleSheet("font-family: monospace")
        self.addWidget(self.console)
        self.macroModel = macroModel
        self.already_connected = set()
        # XXX TODO check
        #for k in macroModel.instances:
        #    self.connect(k)

    def setupToolBars(self, tb, parent):
        tb.addAction(icons.start, "Run", self.onRun)
        tb.addAction(icons.save, "Save", self.onSave)

    def eventFilter(self, object, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.teEditor.textCursor().insertText("    ")
                return True
        return False

    def closeEvent(self, event):
        event.accept()

    def connect(self, macro):
        if macro not in self.already_connected:
            getDevice(macro).boxvalue.printno.signalUpdateComponent.connect(
                self.appendConsole)
            self.already_connected.add(macro)

    def appendConsole(self, box, value, ts):
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(box.configuration.value.print)

    def initReply(self, ok, error):
        self.console.moveCursor(QTextCursor.End)
        self.console.insertPlainText(error)

    def onRun(self):
        self.console.clear()
        try:
            compile(self.teEditor.toPlainText(), self.macroModel.title, "exec")
        except SyntaxError as e:
            if e.filename[7:-3] == self.macroModel.title:
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
            getDevice(self.macroModel.instanceId).signalInitReply.connect(
                self.initReply)
            self.macroModel.run()

    def onSave(self):
        fn = getSaveFileName(
                caption="Save Macro to File",
                filter="Python files (*.py)",
                suffix="py",
                selectFile=self.macroModel.title + ".py")
        if not fn:
            return

        with open(fn, "w") as out:
            out.write(self.teEditor.toPlainText())

    def onMacroChanged(self):
        # XXX: TODO: this can be removed once a traits macro model exists
        # which updates the project whenever changes appear
        # self.macro.project.setModified(True)
        print("TODO: project needs to be modified")
