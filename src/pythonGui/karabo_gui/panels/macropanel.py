#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


""" This is the central panel to edit macros. """

__all__ = ["MacroPanel"]


from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.topology import getDevice
from karabo_gui.util import getSaveFileName

from PyQt4.QtCore import Qt, pyqtSignal, QEvent
from PyQt4.QtGui import (QTextEdit, QPlainTextEdit, QMessageBox,
                         QSplitter, QTextCursor)

from IPython.qt.console.pygments_highlighter import PygmentsHighlighter

class MacroPanel(Dockable, QSplitter):
    signalSave = pyqtSignal(str, str)


    def __init__(self, macro):
        QSplitter.__init__(self, Qt.Vertical)
        self.edit = QTextEdit(self)
        self.edit.installEventFilter(self)
        self.edit.setAcceptRichText(False)
        self.edit.setStyleSheet("font-family: monospace")
        try:
            self.edit.setPlainText(macro.load())
        except KeyError:
            pass
        PygmentsHighlighter(self.edit.document())
        self.addWidget(self.edit)
        self.edit.setLineWrapMode(QTextEdit.NoWrap)
        self.edit.textChanged.connect(self.onMacroChanged)

        self.console = QPlainTextEdit(self)
        self.console.setReadOnly(True)
        self.console.setStyleSheet("font-family: monospace")
        self.addWidget(self.console)
        self.macro = macro
        self.already_connected = set()
        for k in macro.instances:
            self.connect(k)

    def setupToolBars(self, tb, parent):
        tb.addAction(icons.start, "Run", self.onRun)
        tb.addAction(icons.save, "Save", self.onSave)


    def eventFilter(self, object, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.edit.textCursor().insertText("    ")
                return True
        return False

    def closeEvent(self, event):
        # Widgets get destroyed with this event
        self.macro.editor = None
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
            compile(self.edit.toPlainText(), self.macro.name, "exec")
        except SyntaxError as e:
            if e.filename[7:-3] == self.macro.name:
                c = self.edit.textCursor()
                c.movePosition(c.Start)
                c.movePosition(c.Down, n=e.lineno - 1)
                c.movePosition(c.Right, n=e.offset)
                self.edit.setTextCursor(c)
            QMessageBox.warning(self.edit, type(e).__name__,
                                "{}\n{}{}^\nin {} line {}".format(
                                e.msg, e.text, " " * e.offset, e.filename,
                                e.lineno))
        else:
            getDevice(self.macro.instanceId).signalInitReply.connect(
                self.initReply)
            self.macro.run()


    def onSave(self):
        fn = getSaveFileName("Save Macro to File", suffix="py",
                             filter="Python files (*.py)",
                             selectFile=self.macro.name + ".py")
        if not fn:
            return

        with open(fn, "w") as out:
            out.write(self.edit.toPlainText())


    def onMacroChanged(self):
        # XXX: TODO: this can be removed once a traits macro model exists
        # which updates the project whenever changes appear
        # self.macro.project.setModified(True)
        print("TODO: project needs to be modified")
