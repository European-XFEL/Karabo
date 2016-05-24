from karabo.middlelayer_api.hash import Hash


class MonitorData(object):
    """ This class represents a data structure which is needed for the later
    run-control.
    """

    def __init__(self, name, config=None):
        super(MonitorData, self).__init__()

        self.name = name
        self.config = config

    @classmethod
    def deserialize(cls, name, data):
        """ This function loads the object from a serialized representation.
        """
        config = Hash.decode(data, 'XML')
        return cls(name, config=config)

    def serialize(self):
        """ This function serializes the object to an array of bytes.
        """
        return self.config.encode('XML')
