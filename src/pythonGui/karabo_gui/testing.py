import contextlib
import sys
import unittest

from PyQt4.QtCore import QEventLoop
from PyQt4.QtGui import QApplication

from karabo_gui import icons
import karabo_gui.singletons.api as singletons_mod


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases
    """

    def setUp(self):
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app

        icons.init()

    def process_qt_events(self):
        # Give the event loop 10ms to process its events
        self.app.processEvents(QEventLoop.AllEvents, 10)


@contextlib.contextmanager
def assert_trait_change(obj, name):
    """A context manager which watches for traits events
    """
    events = []

    def handler():
        events.append(None)

    obj.on_trait_change(handler, name)
    try:
        yield
    finally:
        obj.on_trait_change(handler, name, remove=True)
        if len(events) == 0:
            msg = 'Expected change for trait "{}" was not observed!'
            raise AssertionError(msg.format(name))


@contextlib.contextmanager
def singletons(**objects):
    """Provide a collection of singletons to be used for the duration of a
    `with`-block.
    """
    # XXX: Yes, we're being naughty here. It's for testing though...
    singletons_dict = singletons_mod.__singletons
    # Remember what got replaced
    replaced = {k: singletons_dict[k] for k in objects if k in singletons_dict}
    try:
        # Replace and yield
        for key, obj in objects.items():
            singletons_dict[key] = obj
        yield
    finally:
        # Put things back as they were
        for key, obj in replaced.items():
            singletons_dict[key] = obj
        for key in objects:
            if key not in replaced:
                del singletons_dict[key]
