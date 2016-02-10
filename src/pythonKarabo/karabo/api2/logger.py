from bisect import bisect
from contextlib import contextmanager
from datetime import datetime
import logging
import traceback

from .hash import Hash, StringList
from .schema import Configurable, ListOfNodes


class _Filter(logging.Filter):
    def __init__(self, parent):
        super().__init__()
        self.parent = parent

    def filter(self, *args, **kwargs):
        return self.parent.filter(*args, **kwargs)


class Filter(Configurable):
    def __init__(self, config, parent, key):
        super().__init__(config, parent, key)
        self.filter = _Filter()
        self.filter.parent = self


class Handler(Configurable):
    filters = ListOfNodes(Filter,
                          description="Filters for this handler",
                          displayedName="Filters",
                          defaultValue=StringList())

    def __init__(self, config, parent, key):
        super().__init__(config, parent, key)
        self.handler = _Handler(self)
        for f in self.filters:
            self.handler.addFilter(f.filter)
        self.parent = parent


class _Handler(logging.Handler):
    def __init__(self, parent):
        super().__init__()
        self.parent = parent

    def emit(self, *args, **kwargs):
        return self.parent.emit(*args, **kwargs)


class NetworkHandler(Handler):
    def emit(self, record):
        hash = Hash(
            "timestamp", datetime.fromtimestamp(record.created).isoformat(),
            "type", ("DEBUG", "INFO", "WARN", "ERROR"
                     )[bisect([20, 30, 40], record.levelno)],
            "category", self.parent.broker.deviceId,
            "message", record.getMessage(),
            "lineno", record.lineno,
            "module", record.module,
            "funcname", record.funcName)
        if record.exc_info is not None:
            hash["traceback"] = traceback.format_exception(*record.exc_info)
        self.parent.broker.log(hash)


class PrintHandler(Handler):
    def emit(self, record):
        print("---------- Logger start -----------")
        print(self.handler.format(record))
        print("---------- Logger end -----------")


class Logger(Configurable):
    handlers = ListOfNodes(
        Handler,
        description="Handlers for logging",
        displayedName="Handlers",
        defaultValue=["NetworkHandler", "PrintHandler"])

    filters = ListOfNodes(
        Filter,
        description="Global filters for logging",
        displayedName="Filters", defaultValue=StringList())

    @contextmanager
    def setBroker(self, broker):
        """Once a device is up and running, set the broker

        This method adds the log handlers and filters defined for a device
        to the Python logging loggers. This can only be done once we
        have a connection to the broker, and should stop once we loose it."""
        self.logger = logging.getLogger(broker.deviceId)
        for h in self.handlers:
            self.logger.addHandler(h.handler)
        for f in self.filters:
            self.logger.addFilter(f.filter)
        self.broker = broker
        try:
            yield
        finally:
            for h in self.handlers:
                self.logger.removeHandler(h.handler)
            for f in self.filters:
                self.logger.removeFilter(f.filter)

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
