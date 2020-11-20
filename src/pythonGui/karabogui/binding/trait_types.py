from traits.api import Range


class NumpyRange(Range):
    """A fast-validating trait type who casts to a `numpy` dtype on validate
    """

    def __init__(self, dtype=None, **kwargs):
        self._dtype = dtype
        super(NumpyRange, self).__init__(**kwargs)

    def validate(self, object, name, value):
        """ Validate that the value is in the specified range.

        Note: Once the value is validated we cast to the numpy dtype. This is
        safe!
        """
        ret = super().validate(object, name, value)
        return self._dtype(ret)
