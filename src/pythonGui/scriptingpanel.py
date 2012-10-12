#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the scripting panel of the bottom
   middle of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["ScriptingPanel", "PythonConsole"]

import re
import sys
import code

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class ScriptingPanel(QWidget):


    def __init__(self):
        super(ScriptingPanel, self).__init__()
        
        self.__teConsole = QTextEdit(self)#PythonConsole(self)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__teConsole)


    def setupActions(self):
        pass


    def setupToolBar(self, toolBar):
        pass


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass



class PythonConsole(QTextEdit):


    class InteractiveInterpreter(code.InteractiveInterpreter):


        def __init__(self, locals):
            code.InteractiveInterpreter.__init__(self, locals)


        def runIt(self, command):
            code.InteractiveInterpreter.runsource(self, command)


    def __init__(self, parent):
        super(PythonConsole, self).__init__(parent)

        sys.stdout = self
        sys.stderr = self
        self.__multiLine = False # code spans more than one line
        self.__command = '' # command to be ran
        self.__hasLine = False
        
        self.__history = [] # list of commands entered
        self.__historyIndex = -1
        self.__interpreterLocals = {}

        self.printBanner() # print sys info
        self.marker() # make the >>> or ... marker

        # Setting the font
        self.setFont(QFont('Courier', 12))

        # Initialize interpreter with self locals
        self.initInterpreter(locals())
        
        # Import stuff from pyexfel
        self.interpreter.runIt("from libkarabo import *")


    def printBanner(self):
        self.write(sys.version + '\n')
        #self.write(' on ' + sys.platform + '\n')
        self.write('PyQt4 ' + PYQT_VERSION_STR + '\n')
        msg = 'Type !hist for a history view and !hist(n) history index recall'
        self.write(msg + '\n')
        

    def marker(self):
        if self.__multiLine:
            self.insertPlainText('... ')
        else:
            self.insertPlainText('>>> ')


    def initInterpreter(self, interpreterLocals=None):
        if interpreterLocals:
            # when we pass in locals, we don't want it to be named "self"
            # so we rename it with the name of the class that did the passing
            # and reinsert the locals back into the interpreter dictionary
            selfName = interpreterLocals['self'].__class__.__name__
            interpreterLocalVars = interpreterLocals.pop('self')
            self.__interpreterLocals[selfName] = interpreterLocalVars
        else:
            self.__interpreterLocals = interpreterLocals
        self.interpreter = self.InteractiveInterpreter(self.__interpreterLocals)


    def updateInterpreterLocals(self, newLocals):
        className = newLocals.__class__.__name__
        self.__interpreterLocals[className] = newLocals


    def write(self, line):
        self.insertPlainText(line)
        self.ensureCursorVisible()


    def clearCurrentBlock(self):
        # block being current row
        length = len(self.document().lastBlock().text()[4:])
        if length == 0:
            return None
        else:
            # should have a better way of doing this but I can't find it
            [self.textCursor().deletePreviousChar() for x in xrange(length)]
        return True


    def recallHistory(self):
        # used when using the arrow keys to scroll through history
        self.clearCurrentBlock()
        if self.__historyIndex <> -1:
            self.insertPlainText(self.__history[self.__historyIndex])
        return True


    def customCommands(self, command):
        
        if command == '!hist': # display history
            self.append('') # move down one line
            # vars that are in the command are prefixed with ____CC and deleted
            # once the command is done so they don't show up in dir()
            backup = self.__interpreterLocals.copy()
            history = self.__history[:]
            history.reverse()
            for i, x in enumerate(history):
                iSize = len(str(i))
                delta = len(str(len(history))) - iSize
                line = line = ' ' * delta + '%i: %s' % (i, x) + '\n'
                self.write(line)
            self.updateInterpreterLocals(backup)
            self.marker()
            return True

        if re.match('!hist\(\d+\)', command): # recall command from history
            backup = self.__interpreterLocals.copy()
            history = self.__history[:]
            history.reverse()
            index = int(command[6:-1])
            self.clearCurrentBlock()
            command = history[index]
            if command[-1] == ':':
                self.__multiLine = True
            self.write(command)
            self.updateInterpreterLocals(backup)
            return True

        return False


    def keyPressEvent(self, event):

        if event.key() == Qt.Key_Escape:
            # proper exit
            self.interpreter.runIt('exit()')

        if event.key() == Qt.Key_Down:
            if self.__historyIndex == len(self.__history):
                self.__historyIndex -= 1
            try:
                if self.__historyIndex > -1:
                    self.__historyIndex -= 1
                    self.recallHistory()
                else:
                    self.clearCurrentBlock()
            except:
                pass
            return None

        if event.key() == Qt.Key_Up:
            try:
                if len(self.__history) - 1 > self.__historyIndex:
                    self.__historyIndex += 1
                    self.recallHistory()
                else:
                    self.__historyIndex = len(self.__history)
            except:
                pass
            return None

        if event.key() == Qt.Key_Home:
            # set cursor to position 4 in current block. 4 because that's where
            # the marker stops
            blockLength = len(self.document().lastBlock().text()[4:])
            lineLength = len(self.document().toPlainText())
            position = lineLength - blockLength
            textCursor = self.textCursor()
            textCursor.setPosition(position)
            self.setTextCursor(textCursor)
            return None

        if event.key() in [Qt.Key_Left, Qt.Key_Backspace]:
            # don't allow deletion of marker
            if self.textCursor().positionInBlock() == 4:
                return None

        if event.key() in [Qt.Key_Return, Qt.Key_Enter]:
            # set cursor to end of line to avoid line splitting
            textCursor = self.textCursor()
            position = len(self.document().toPlainText())
            textCursor.setPosition(position)
            self.setTextCursor(textCursor)

            line = str(self.document().lastBlock().text())[4:] # remove marker
            line.rstrip()
            self.__historyIndex = -1

            if self.customCommands(line):
                return None
            else:
                try:
                    line[-1]
                    self.__hasLine = True
                    if line[-1] == ':':
                        self.__multiLine = True
                    self.__history.insert(0, line)
                except:
                    self.__hasLine = False

                if self.__hasLine and self.__multiLine: # multi line command
                    self.__command += line + '\n' # + command and line
                    self.append('') # move down one line
                    self.marker() # handle marker style
                    return None

                if self.__hasLine and not self.__multiLine: # one line command
                    self.__command = line # line is the command
                    self.append('') # move down one line
                    self.interpreter.runIt(self.__command)
                    self.__command = '' # clear command
                    self.marker() # handle marker style
                    return None

                if self.__multiLine and not self.__hasLine: # multi line done
                    self.append('') # move down one line
                    self.interpreter.runIt(self.__command)
                    self.__command = '' # clear command
                    self.__multiLine = False # back to single line
                    self.marker() # handle marker style
                    return None

                if not self.__hasLine and not self.__multiLine: # just enter
                    self.append('')
                    self.marker()
                    return None
                return None
                
        # Allow all other key events
        super(PythonConsole, self).keyPressEvent(event)

