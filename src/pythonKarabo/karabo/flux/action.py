
class BaseAction(object):
    """ The base class for all action classes.
    """

    # A required class attribute used to determine an action's type
    # Not every action class needs to define a type; it should be a general
    # classification, not a specific one (except where specificity is needed).
    action_type = 'BaseAction'
