import inspect
import os.path as op
import pdb

from PyQt4.QtCore import pyqtRemoveInputHook


def set_trace():
    """Set a breakpoint in the Python debugger that works with PyQt.
    """
    pyqtRemoveInputHook()
    pdb.set_trace()


def print_stack_trace(num_frames=0):
    """Print a stack trace, without requiring an exception to have been thrown.
    """
    frames = inspect.stack()
    IGNORED = 2  # The frames including this function and inspect.stack()
    num_frames = num_frames if num_frames != 0 else (len(frames) - IGNORED)
    frames_subset = frames[IGNORED:IGNORED+num_frames]
    try:
        print()
        for tup in frames_subset[::-1]:
            fname, line, function, context = tup[1:-1]
            print('{}:{}:{}\n{}'.format(op.basename(fname), function, line,
                                        context[0].strip()))
    finally:
        del frames
        del frames_subset
