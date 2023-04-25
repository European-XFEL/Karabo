# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from enum import Enum
from functools import total_ordering
from heapq import heappop, heappush

from qtpy.QtCore import QObject, QRunnable, QThreadPool, QTimer, Signal, Slot


class TaskMediator(QObject):
    finished = Signal(object)


class TaskResult:
    __slots__ = ("data", )

    def __init__(self, data=None):
        self.data = data

    def result(self):
        if isinstance(self.data, Exception):
            raise self.data
        return self.data

    def exception(self):
        if isinstance(self.data, Exception):
            return self.data
        return None


class Task(QRunnable):
    """A task that can be run the global instance of `QThreadPool`"""

    def __init__(self, obj, *args):
        super().__init__()
        self.obj = obj
        self.args = args
        self.signaller = None

    def run(self):
        """Reimplemented method of `QRunnable`"""
        try:
            data = self.obj(*self.args)
        except Exception as exc:
            data = exc
        if self.signaller is not None:
            result = TaskResult(data)
            self.signaller.finished.emit(result)
            self.signaller.deleteLater()

    def connect(self, callback):
        self.signaller = TaskMediator()
        self.signaller.finished.connect(callback)


def background(obj, *args, callback=None):
    """Run a function in the background in the global threadpool

    :returns: Returns task (QRunnable)
    """

    task = Task(obj, *args)
    if callback is not None:
        task.connect(callback)
    QThreadPool.globalInstance().start(task)
    return task


@total_ordering
class Priority(Enum):
    NETWORK = 0
    BIG_DATA = 1
    BACKGROUND = 2

    def __lt__(self, other):
        if isinstance(other, Priority):
            return self.value < other.value
        else:
            return NotImplemented


def executeLater(task, priority):
    """append a task to the queue of the ones to be executed"""
    global counter
    heappush(queue, (priority, counter, task))
    counter += 1
    timer.start()


MAX_ITEM_PROCESSING = 5
queue = []
counter = 0
timer = None


@Slot()
def timeout():
    """execute up to ``MAX_ITEM_PROCESSING`` tasks if existing

    If big data is in the stack, we cut there!
    """
    task_counter = MAX_ITEM_PROCESSING
    while queue and task_counter > 0:
        prio, _, task = heappop(queue)
        task()
        if prio.value > 0:
            # We found big data and break!
            break
        task_counter -= 1

    if not queue:
        timer.stop()


def create_background_timer():
    """Manually create the background `timer` after a QApplication has
    been created"""
    global timer
    timer = QTimer()
    timer.setInterval(0)
    timer.timeout.connect(timeout)
