def get_integers(names, element):
    """ Read a list of integer values from an `Element` instance.
    """
    return {name: int(element.get(name)) for name in names}


def set_integers(names, model, element):
    """ Copy a list of integer attribute values to an `Element` instance.

    This is the inverse of `_get_numbers`.
    """
    for name in names:
        element.set(name, str(getattr(model, name)))
