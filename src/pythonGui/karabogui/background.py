from enum import Enum
from functools import total_ordering
from heapq import heappop, heappush

from qtpy.QtCore import QTimer, Slot


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
