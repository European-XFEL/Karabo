from bisect import bisect
import logging
import time
import weakref

from karabo.enums import Assignment
from karabo.hash import Hash
from karabo.hashtypes import String, StringList
from karabo.schema import Configurable, ListOfNodes


class _Filter(logging.Filter):
    def __init__(self, parent):
        super().__init__()
        self.parent = weakref.ref(parent, self.remove)
        self.added = []

    def filter(self, *args, **kwargs):
        return self.parent().filter(*args, **kwargs)

    def remove(self, parent):
        for a in added:
            a.removeFilter(self)

    def addTo(self, what):
        self.added.append(what)
        what.addFilter(self)


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
            f.filter.addTo(self.handler)
        self.parent = parent


class _Handler(logging.Handler):
    def __init__(self, parent):
        super().__init__()
        self.parent = weakref.ref(parent, self.remove)
        self.added = []

    def emit(self, *args, **kwargs):
        return self.parent().emit(*args, **kwargs)

    def remove(self, parent):
        for a in self.added:
            a.removeHandler(self)

    def addTo(self, what):
        self.added.append(what)
        what.addHandler(self)


class NetworkHandler(Handler):
    def emit(self, record):
        self.parent.broker.log("{} | {} | {} | {}#".format(
            time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(record.created)),
            ["DEBUG", "INFO", "WARN", "ERROR"][
                bisect([20, 30, 40], record.levelno)],
            self.parent.broker.deviceId,
            record.getMessage()))


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

    def setBroker(self, broker):
        self.logger = logging.getLogger(broker.deviceId)
        self.broker = broker
        for h in self.handlers:
            h.handler.addTo(self.logger)
        for f in self.filters:
            f.filter.addTo(self.logger)

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
