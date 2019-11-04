from enum import Enum
from functools import total_ordering
from heapq import heappush, heappop

from PyQt5.QtCore import QTimer


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


def timeout():
    """execute up to ``MAX_ITEM_PROCESSING`` tasks if existing

    If big data is in the stack, we cut there!
    """
    items = min(len(queue), MAX_ITEM_PROCESSING)
    while items > 0:
        prio, _, task = heappop(queue)
        task()
        if prio.value > 0:
            break
        items -= 1
    if not queue:
        timer.stop()


queue = []
counter = 0
timer = QTimer()
timer.setInterval(0)
timer.timeout.connect(timeout)
