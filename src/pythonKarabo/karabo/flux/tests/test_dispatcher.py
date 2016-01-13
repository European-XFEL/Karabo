from nose.tools import with_setup

from ..action import BaseAction
from ..dispatcher import (addHandler, removeHandler, submitAction,
                          submitActionSync, _clear_handlers)
from ..handler import BaseHandler


class MyBaseAction(BaseAction):
    action_type = 'mine'


class MyActionA(MyBaseAction):
    pass


class MyActionB(MyBaseAction):
    action_type = 'action_b'


class CountingHandler(BaseHandler):
    def __init__(self, counts, sync_action=None):
        self.counts = counts
        self.sync_action = sync_action

    def handle(self, action):
        self.counts[0] += 1

    def handleSync(self, action):
        self.counts[0] += 1
        return self.sync_action()


def test_add_remove():
    handler0 = CountingHandler([])
    handler1 = CountingHandler([])

    addHandler(MyActionA, handler0)
    removeHandler(MyActionA, handler0)

    addHandler('foo', handler1)
    removeHandler('foo', handler1)


def test_submit():
    counts = [0]
    handler = CountingHandler(counts)
    addHandler(MyActionA, handler)

    submitAction(MyActionA())
    assert counts == [1]


@with_setup(setup=_clear_handlers)
def test_submit_sync():
    counts = [0]
    handler = CountingHandler(counts, sync_action=MyActionA)
    addHandler(MyActionA, handler)

    submitActionSync(MyActionA())
    assert counts == [2]


def test_multiple_handlers():
    a_counts = [0]
    a_handler = CountingHandler(a_counts)
    addHandler(MyActionA, a_handler)

    b_counts = [0]
    b_handler = CountingHandler(b_counts)
    addHandler(MyActionB, b_handler)

    submitAction(MyActionA())
    assert a_counts == [1] and b_counts == [0]

    a_counts[0] = 0
    submitAction(MyActionB())
    assert a_counts == [0] and b_counts == [1]
