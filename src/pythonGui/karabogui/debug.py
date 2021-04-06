from functools import wraps
import inspect
import os.path as op
import pdb
from time import perf_counter

from qtpy.QtCore import pyqtRemoveInputHook

from karabogui.util import process_qt_events


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
    frames_subset = frames[IGNORED:IGNORED + num_frames]
    try:
        print()
        for tup in frames_subset[::-1]:
            fname, line, function, context = tup[1:-1]
            print('{}:{}:{}\n{}'.format(op.basename(fname), function, line,
                                        context[0].strip()))
    finally:
        del frames
        del frames_subset


def timeit(process_events=True):
    """We now call timeit with arguments because functions hooked to
       paintEvents cannot be timed with invoked processEvents. This decorator
       is now used as a called function:

       @timeit()
       def foo():
           ...

       @timeit(process_events=False)
       def paintEvent():
           ..."""

    def decorator(func):
        """A timing decorator for the GUI"""

        @wraps(func)
        def wrapper(*args, **kwargs):
            # Empty the eventloop stack before!
            if process_events:
                process_qt_events(timeout=10)

            t_start = perf_counter()
            ret = func(*args, **kwargs)

            # But process the generated eventloop stack for the performance
            # measurement!
            if process_events:
                process_qt_events()

            elapsed = perf_counter() - t_start
            print("{} took {}".format(func.__name__, elapsed))
            return ret

        return wrapper

    return decorator
