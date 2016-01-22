import pdb

from PyQt4.QtCore import pyqtRemoveInputHook


def set_trace():
    """ Set a tracepoint in the Python debugger that works with PyQt.
    """
    pyqtRemoveInputHook()
    pdb.set_trace()
