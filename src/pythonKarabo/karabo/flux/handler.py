from abc import ABCMeta, abstractmethod


class BaseHandler(object, metaclass=ABCMeta):
    """ An abstract base class for handlers; objects which do the work of
    responding to actions.
    """

    @abstractmethod
    def handle(self, action):
        """ Handle a single action.
        """

    @abstractmethod
    def handleSync(self, action):
        """ Handle a single action synchronously.
        """
