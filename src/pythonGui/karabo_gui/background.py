"""This module allows to run code with low priority"""

from enum import Enum
from functools import total_ordering
from heapq import heappush, heappop

from PyQt4.QtCore import QTimer


@total_ordering
class Priority(Enum):
    NETWORK = 0
    BACKGROUND = 1

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


def timeout():
    """execute one task if existing"""
    _, _, task = heappop(queue)
    if not queue:
        timer.stop()
    task()


queue = []
counter = 0
timer = QTimer()
timer.setInterval(0)
timer.timeout.connect(timeout)
