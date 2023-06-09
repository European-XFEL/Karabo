# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import ast
import collections
import functools
import logging
import os
from asyncio import get_event_loop
from contextlib import contextmanager
from pathlib import Path

import pytest


def get_ast_objects(package, ignore=[]):
    """Get all ast objects from a specified package

    :param ignore: list of filenames that are ignored in the lookup
    """
    def _get_ast(path):
        """Get an ast.AST object for the specified file"""
        with open(path, 'rb') as fp:
            return compile(fp.read(), path, "exec", ast.PyCF_ONLY_AST)

    common_dir = str(Path(package.__file__).parent)
    ast_objects = []
    for dirpath, _, filenames in os.walk(common_dir):
        for fn in filenames:
            if Path(fn).suffix == ".py" and fn not in ignore:
                path = str(Path(dirpath).joinpath(fn))
                ast_objects.append(_get_ast(path))
    return ast_objects


def run_test(func):
    """Run a pytest test function `func` in the karabo eventloop"""

    @functools.wraps(func)
    @pytest.mark.asyncio
    async def wrapper(*args, **kwargs):
        loop = get_event_loop()
        lead = getattr(loop, "lead", None)
        task = loop.create_task(loop.run_coroutine_or_thread(
            func, *args, **kwargs), instance=lead)
        return await task

    return wrapper


_LoggingWatcher = collections.namedtuple("_LoggingWatcher",
                                         ["records", "output"])


class _CapturingHandler(logging.Handler):
    """A logging handler capturing all (raw and formatted) logging output.
    """

    def __init__(self):
        logging.Handler.__init__(self)
        self.watcher = _LoggingWatcher([], [])

    def flush(self):
        pass

    def emit(self, record):
        self.watcher.records.append(record)
        msg = self.format(record)
        self.watcher.output.append(msg)


@contextmanager
def assertLogs(logger_name=None, level=None):
    """A context manager used to implement assertLogs().

    :param logger_name: defaults to `None` (root)
    :param level: The logging level, defaults to `None` -> INFO
    """
    try:
        if isinstance(logger_name, logging.Logger):
            logger = logger_name
        else:
            logger = logging.getLogger(logger_name)

        if level:
            level = logging._nameToLevel.get(level, level)
        else:
            level = logging.INFO

        # Store old
        old_handlers = logger.handlers[:]
        old_level = logger.level
        old_propagate = logger.propagate
        # Attach new
        LOGGING_FORMAT = "%(levelname)s:%(name)s:%(message)s"
        formatter = logging.Formatter(LOGGING_FORMAT)
        handler = _CapturingHandler()
        handler.setFormatter(formatter)
        watcher = handler.watcher
        logger.handlers = [handler]
        logger.setLevel(level)
        logger.propagate = False

        yield handler.watcher
    finally:
        logger.handlers = old_handlers
        logger.propagate = old_propagate
        logger.setLevel(old_level)

        if len(watcher.records) == 0:
            raise AssertionError(
                "no logs of level {} or higher triggered on {}"
                .format(logging.getLevelName(level), logger.name))
