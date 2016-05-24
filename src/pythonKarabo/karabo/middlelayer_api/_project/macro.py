

class MacroData(object):
    """ A simple macro object which supports file round-trips.
    """
    def __init__(self, name, code=''):
        self.name = name
        self.code = code
        self.instanceId = "Macro-{}".format(self.name)

    @classmethod
    def deserialize(cls, name, data):
        return cls(name, code=data)

    def serialize(self):
        return self.code
