from ..action import BaseAction
from ..dispatcher import (addHandler, removeHandler, submitAction,
                          submitActionSync)
from ..handler import BaseHandler


class MyBaseAction(BaseAction):
    klass = 'mine'


class MyAction(MyBaseAction):
    pass


class MyHandler(BaseHandler):
    def __init__(self):
        self.handles = 0

    def handle(self, action):
        self.handles += 1

    def handleSync(self, action):
        self.handles += 1
        return MyAction()


def test_add_remove():
    handler = MyHandler()
    addHandler(MyAction, handler)
    removeHandler('mine', handler)


def test_submit():
    handler = MyHandler()
    addHandler(MyAction, handler)

    submitAction(MyAction())
    assert handler.handles == 1

    submitActionSync(MyAction())
    assert handler.handles == 3
