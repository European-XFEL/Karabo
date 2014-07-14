#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a message box with different
   static methods to either show an alarm, an error, ...
"""

__all__ = ["MessageBox"]


from PyQt4.QtCore import QObject
from PyQt4.QtGui import QMessageBox


class MessageBox(QObject):
    
    def __init__(self):
        super(MessageBox, self).__init__()


    @staticmethod
    def showAlarm(text, title=None):
        QMessageBox.critical(None, "Alarm", text)


    @staticmethod
    def showError(text, title=None):
        QMessageBox.critical(None, "Error", text)


    @staticmethod
    def showInformation(text, title=None):
        QMessageBox.information(None, "Information", text)


    @staticmethod
    def showQuestion(question, title=None):
        result = QMessageBox.question(None, "Question", question, QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
        return result


    @staticmethod
    def showWarning(text, title=None):
        QMessageBox.warning(None, "Warning", text)

