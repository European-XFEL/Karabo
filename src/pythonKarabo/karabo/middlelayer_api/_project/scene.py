

class SceneData(object):
    """ A simple scene object which supports round-trips between project files.
    """
    def __init__(self, name, xmlData=None):
        self.name = name
        self.__xmlData = xmlData

    @classmethod
    def deserialize(cls, name, data):
        """ This function loads the object from a serialized representation.
        """
        return cls(name, xmlData=data)

    def serialize(self):
        """ This function serializes the object to an array of bytes.
        """
        return self.__xmlData
