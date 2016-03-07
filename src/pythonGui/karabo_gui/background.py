"""This module allows to run code with low priority"""

from collections import deque


queue = deque()


def executeLater(task):
    """append a task to the queue of the ones to be executed"""
    queue.append(task)
    callback()


def execute():
    """execute one task if existing

    returns True if a task was executed, False if there was none."""
    if queue:
        t = queue.popleft()
        t()
        return True
    else:
        return False


def initialize(cb):
    """Initialize the callback of this module

    the callback *cb* will be called once a task gets added to the queue"""
    global callback
    callback = cb
