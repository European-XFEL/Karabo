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
import logging
import traceback
from bisect import bisect
from collections import deque
from contextlib import contextmanager
from datetime import datetime

from karabo.native import (
    AccessLevel, AccessMode, Configurable, Hash, String, VectorString)


class CacheLog:
    """The CacheLog is an internal singleton collecting the print logs"""
    events = deque(maxlen=100)

    @classmethod
    def add_event(cls, entry):
        cls.events.append(entry)

    @classmethod
    def summary(cls, num):
        """Get the summary of the last `num` log entries"""
        # Make a list for slicing
        events = list(cls.events)[-num:]
        return events


class CacheHandler(logging.Handler):
    def emit(self, record):

        level = ("DEBUG", "INFO", "WARN", "ERROR", "FATAL"
                 )[bisect([20, 30, 40, 50], record.levelno)]
        timestamp = datetime.fromtimestamp(record.created).isoformat()
        message = record.getMessage()
        deviceId = self.parent.broker.deviceId
        trace = ""
        if record.exc_info is not None:
            trace = "".join(traceback.format_exception(*record.exc_info))

        CacheLog.add_event(
            Hash("type", level, "message", message,
                 "timestamp", timestamp, "category", deviceId,
                 "traceback", trace))


class PrintHandler(logging.Handler):
    def emit(self, record):
        print("---------- Logger start -----------")
        # record.levelno is 10, 20, 30,... for "DEBUG", "INFO", "WARN",...
        level = ("DEBUG", "INFO", "WARN", "ERROR", "FATAL"
                 )[bisect([20, 30, 40, 50], record.levelno)]
        timestamp = datetime.fromtimestamp(record.created)
        deviceId = self.parent.broker.deviceId
        message = self.format(record)
        print(timestamp, level, deviceId)
        print(message)
        print("---------- Logger end -----------")


_LOGGER_HANDLER = {
    "CacheHandler": CacheHandler,
    "PrintHandler": PrintHandler
}


def build_logger_node(handles=None):
    """Create a `Logger` Configurable for a Node

    :param handles: List of handlers (str) used for the logger.
                    Choices: `CacheHandler`, `PrintHandler`
    """
    if handles is None:
        handles = ["CacheHandler", "PrintHandler"]

    class Logger(Configurable):
        logger = None

        handlers = VectorString(
            defaultValue=handles,
            requiredAccessLevel=AccessLevel.EXPERT,
            accessMode=AccessMode.READONLY)

        @String(
            displayedName="Logging Level",
            options=("DEBUG", "INFO", "WARN", "ERROR", "FATAL"),
            requiredAccessLevel=AccessLevel.EXPERT,
            defaultValue="INFO")
        def level(self, value):
            """The minimum level for this logger to log"""
            if self.logger is not None:
                self.logger.setLevel(value)
            self.level = value

        @contextmanager
        def setBroker(self, broker):
            """Once a device is up and running, set the broker

            This method adds the log handlers and filters defined for a
            device to the Python logging loggers. This can only be done
            once we have a connection to the broker, and should stop once
            we loose it."""
            self.logger = logging.getLogger(broker.deviceId)
            self.logger.setLevel(self.level)
            for handler in self.handlers.value:
                h = _LOGGER_HANDLER[handler]()
                h.parent = self
                self.logger.addHandler(h)
            self.broker = broker
            try:
                yield
            finally:
                self.logger.handlers = []

        def INFO(self, what):
            """ legacy """
            self.logger.info(what)

        def WARN(self, what):
            """ legacy """
            self.logger.warning(what)

        def DEBUG(self, what):
            """ legacy """
            self.logger.debug(what)

        def ERROR(self, what):
            """ legacy """
            self.logger.error(what)

    return Logger
