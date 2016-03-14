"""This module allows to run code with low priority"""

from enum import Enum

from PyQt4.QtCore import QCoreApplication, QEvent, QObject


class Priority(Enum):
    NETWORK = -1
    BACKGROUND = -2


class DeferredCallEvent(QEvent):
    type_ = QEvent.registerEventType()

    def __init__(self, callback):
        super().__init__(self.type_)
        self.callback = callback


class DeferredCaller(QObject):
    def customEvent(self, event):
        if isinstance(event, DeferredCallEvent):
            event.callback()

caller = DeferredCaller()


def executeLater(task, priority):
    """append a task to the queue of the ones to be executed"""
    QCoreApplication.postEvent(caller, DeferredCallEvent(task), priority.value)
