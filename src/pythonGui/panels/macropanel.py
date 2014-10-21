#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


""" This is the central panel to edit macros. """

__all__ = ["MacroPanel"]


import icons
from network import Network
from toolbar import ToolBar
from finders import MacroContext

from PyQt4.QtCore import Qt, pyqtSignal
from PyQt4.QtGui import (QWidget, QTextEdit, QHBoxLayout, QMessageBox)



class MacroPanel(QWidget):
    signalSave = pyqtSignal(str, str)


    def __init__(self, macro):
        super(MacroPanel, self).__init__()
        layout = QHBoxLayout(self)
        self.setLayout(layout)
        self.edit = QTextEdit(self)
        self.edit.setAcceptRichText(False)
        self.edit.setStyleSheet("font-family: monospace")
        self.edit.setPlainText(macro.text)
        layout.addWidget(self.edit)
        self.macro = macro


    def setupToolBars(self, tb, parent):
        tb.addAction(icons.save, "Save", self.onSave)


    def onSave(self):
        self.macro.text = self.edit.toPlainText()
        try:
            with MacroContext(self.macro.project):
                self.macro.run()
        except SyntaxError as e:
            if e.filename[7:-3] == self.macro.name:
                c = self.edit.textCursor()
                c.movePosition(c.Start)
                c.movePosition(c.Down, n=e.lineno - 1)
                c.movePosition(c.Right, n=e.offset)
                self.edit.setTextCursor(c)
            QMessageBox.warning(self.edit, "Syntax Error",
                                "{}{}^\nin {} line {}".format(
                                e.text, " " * e.offset, e.filename, e.lineno))


    def onDock(self):
        pass


    def onUndock(self):
        pass
