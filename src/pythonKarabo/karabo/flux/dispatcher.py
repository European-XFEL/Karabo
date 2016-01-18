
from collections import defaultdict

__handlers = defaultdict(list)


def _clear_handlers():
    """ XXX: THIS IS ONLY TO BE USED BE TESTING CODE.
    """
    from sys import modules
    if 'nose' not in modules:
        raise RuntimeError('This function must only be called when testing!!!')

    # Clear the handlers
    __handlers.clear()


def _get_action_type(obj):
    # obj is either an action class or instance
    if hasattr(obj, 'action_type'):
        return obj.action_type

    # OR a string
    assert isinstance(obj, str)
    return obj


def addHandler(action_class, handler):
    """ Add a handler for actions of type `action_class`.
    """
    action_type = _get_action_type(action_class)
    __handlers[action_type].append(handler)


def removeHandler(action_class, handler):
    """ Remove a previously registered handler.
    """
    action_type = _get_action_type(action_class)
    handlers = __handlers[action_type]
    handlers.remove(handler)


def submitAction(action_inst):
    """ Dispatch a single action using the appropriate handlers.
    """
    handlers = __handlers[_get_action_type(action_inst)]
    for handler in handlers:
        handler.handle(action_inst)


def submitActionSync(action_inst):
    """ Dispatch a single action synchronously using the appropriate handler.
    """
    handlers = __handlers[_get_action_type(action_inst)]
    assert len(handlers) == 1
    handler = handlers[0]

    newAction = handler.handleSync(action_inst)
    assert newAction is not None, 'Synchronous handlers must return an action'
    submitAction(newAction)
