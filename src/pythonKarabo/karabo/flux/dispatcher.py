
from collections import defaultdict

__handlers = defaultdict(list)


def _get_klass_key(klass):
    if hasattr(klass, 'klass'):
        return klass.klass

    assert isinstance(klass, str)
    return klass


def addHandler(klass, handler):
    """ Add a handler for actions of type `klass`.
    """
    klass_key = _get_klass_key(klass)
    __handlers[klass_key].append(handler)


def removeHandler(klass, handler):
    """ Remove a previously registered handler.
    """
    klass_key = _get_klass_key(klass)
    handlers = __handlers[klass_key]
    handlers.remove(handler)


def submitAction(action):
    """ Dispatch a single action using the appropriate handlers.
    """
    handlers = __handlers[_get_klass_key(action)]
    for handler in handlers:
        handler.handle(action)


def submitActionSync(action):
    """ Dispatch a single action synchronously using the appropriate handler.
    """
    handlers = __handlers[_get_klass_key(action)]
    assert len(handlers) == 1
    handler = handlers[0]

    newAction = handler.handleSync(action)
    assert newAction is not None, 'Synchronous handlers must return an action'
    submitAction(newAction)
