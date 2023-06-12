# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import inspect
import os.path as op
import pdb
from functools import wraps
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


class profiler:
    """We now call profiler with arguments because functions hooked to
       paintEvents cannot be timed with invoked processEvents.
       Possible arguments are `name`, `process_events` and `timeout` for the
       event chain. The `timeout` argument is only used if the `process_events`
       setting is True (default).

       @profiler()
       def foo():
           ...

       @profiler(process_events=False, timeout=100)
       def paintEvent():
           ...

       with profiler(name="Image")
           # compute image

    """

    def __init__(self, name=None, process_events=True, timeout=200):
        self.name = name
        self.t_start = None
        self.process_events = process_events
        self.timeout = timeout

    def __enter__(self):
        if self.process_events:
            process_qt_events(timeout=self.timeout)
        self.t_start = perf_counter()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.process_events:
            process_qt_events(timeout=self.timeout)

        elapsed = perf_counter() - self.t_start
        name = f"Block {self.name}: " if self.name is not None else ""
        print(f"{name}time elapsed {elapsed}")

    def __call__(self, func):
        """Decorate a function to profile the execution time"""
        name = func.__name__ if self.name is None else self.name

        @wraps(func)
        def wrapper(*args, **kwargs):
            if self.process_events:
                process_qt_events(timeout=self.timeout)

            t_start = perf_counter()
            ret = func(*args, **kwargs)

            # But process the generated eventloop stack for the performance
            # measurement!
            if self.process_events:
                process_qt_events(timeout=self.timeout)

            elapsed = perf_counter() - t_start
            print(f"{name} took {elapsed}")
            return ret

        return wrapper
