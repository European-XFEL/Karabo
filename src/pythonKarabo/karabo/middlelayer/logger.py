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

from karabo.native import AccessLevel, Configurable, Hash, String


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


class InstanceHandler(logging.Handler):

    instanceId = ""

    def emit(self, record):
        # record.levelno is 10, 20, 30,... for "DEBUG", "INFO", "WARN",...
        level = ("DEBUG", "INFO", "WARN", "ERROR", "FATAL"
                 )[bisect([20, 30, 40, 50], record.levelno)]
        timestamp = datetime.fromtimestamp(record.created)
        message = self.format(record)

        print("---------- Logger start -----------")
        print(timestamp, level, self.instanceId)
        print(message)
        print("---------- Logger end -----------")

        trace = ""
        if record.exc_info is not None:
            trace = "".join(traceback.format_exception(*record.exc_info))

        CacheLog.add_event(
            Hash("type", level,
                 "message", message,
                 "timestamp", timestamp.isoformat(),
                 "category", self.instanceId,
                 "traceback", trace))


def build_logger_node(handler: bool = True):
    """Create a `Logger` Configurable for a Node

    :param handler: Attach an instance handler for the logger
    """

    class Logger(Configurable):
        logger = None

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
        def setInstance(self, instanceId: str):
            """Once an instance is up and running, set the instanceId

            This method adds the log handler. This can only be done
            once we have a connection to the broker, and should stop once
            we loose it."""
            self.logger = logging.getLogger(instanceId)
            self.logger.setLevel(self.level)
            if handler:
                h = InstanceHandler()
                h.instanceId = instanceId
                self.logger.addHandler(h)
            try:
                yield
            finally:
                self.logger.handlers = []

        def INFO(self, text: str):
            """ legacy """
            self.logger.info(text)

        def WARN(self, text: str):
            """ legacy """
            self.logger.warning(text)

        def DEBUG(self, text: str):
            """ legacy """
            self.logger.debug(text)

        def ERROR(self, text: str):
            """ legacy """
            self.logger.error(text)

    return Logger
