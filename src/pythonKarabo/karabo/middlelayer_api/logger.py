from bisect import bisect
from contextlib import contextmanager
from datetime import datetime
import logging
import traceback

from karabo.native.data.enums import AccessMode
from karabo.native.data.hash import Hash, String
from karabo.native.data.schema import Configurable, ListOfNodes


class _Filter(logging.Filter):
    def __init__(self, parent):
        super().__init__()
        self.parent = parent

    def filter(self, *args, **kwargs):
        return self.parent.filter(*args, **kwargs)


class Filter(Configurable):
    def __init__(self, config):
        super().__init__(config)
        self.filter = _Filter(self)


class Handler(Configurable):
    filters = ListOfNodes(Filter,
                          description="Filters for this handler",
                          displayedName="Filters",
                          defaultValue=[])

    def __init__(self, config):
        super().__init__(config)
        self.handler = _Handler(self)
        for f in self.filters:
            self.handler.addFilter(f.filter)


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
            # record.levelno is 10, 20, 30,... for "DEBUG", "INFO", "WARN",...
            "type", ("DEBUG", "INFO", "WARN", "ERROR", "FATAL"
                     )[bisect([20, 30, 40, 50], record.levelno)],
            "category", self.parent.broker.deviceId,
            "message", record.getMessage(),
            "msg", record.msg,
            "lineno", record.lineno,
            "module", record.module,
            "funcname", record.funcName)
        args = Hash()
        for i, arg in enumerate(record.args):
            args["{}{}".format(type(arg).__name__, i)] = arg
        hash["args"] = args
        if record.exc_info is not None:
            trace = "".join(traceback.format_exception(*record.exc_info))
            hash["traceback"] = trace

        self.parent.broker.log(hash)


class PrintHandler(Handler):
    def emit(self, record):
        print("---------- Logger start -----------")
        # record.levelno is 10, 20, 30,... for "DEBUG", "INFO", "WARN",...
        level = ("DEBUG", "INFO", "WARN", "ERROR", "FATAL"
                 )[bisect([20, 30, 40, 50], record.levelno)]
        print(datetime.fromtimestamp(record.created), level,
              self.parent.broker.deviceId)
        print(self.handler.format(record))
        print("---------- Logger end -----------")


class Logger(Configurable):
    logger = None

    handlers = ListOfNodes(
        Handler,
        description="Handlers for logging",
        displayedName="Handlers",
        defaultValue=["NetworkHandler", "PrintHandler"],
        accessMode=AccessMode.READONLY)

    filters = ListOfNodes(
        Filter,
        description="Global filters for logging",
        displayedName="Filters", defaultValue=[])

    @String(
        displayedName="Logging Level",
        options=("DEBUG", "INFO", "WARN", "ERROR", "FATAL"),
        defaultValue="INFO")
    def level(self, value):
        """The minimum level for this logger to log"""
        if self.logger is not None:
            self.logger.setLevel(value)
        self.level = value

    @contextmanager
    def setBroker(self, broker):
        """Once a device is up and running, set the broker

        This method adds the log handlers and filters defined for a device
        to the Python logging loggers. This can only be done once we
        have a connection to the broker, and should stop once we loose it."""
        self.logger = logging.getLogger(broker.deviceId)
        self.logger.setLevel(self.level)
        for h in self.handlers:
            h.parent = self
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
