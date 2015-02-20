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
    def showAlarm(text, title="Alarm"):
        QMessageBox.critical(None, title, text)


    @staticmethod
    def showError(text, title="Error"):
        QMessageBox.critical(None, title, text)


    @staticmethod
    def showInformation(text, title="Information"):
        QMessageBox.information(None, title, text)


    @staticmethod
    def showQuestion(question, title="Question"):
        result = QMessageBox.question(None, title, question, QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
        return result


    @staticmethod
    def showWarning(text, title="Warning"):
        QMessageBox.warning(None, title, text)

