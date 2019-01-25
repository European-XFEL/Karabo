import numpy

from .basetypes import NoneValue, QuantityValue
from .enums import NodeType
from .hash import (
    AccessMode, Bool, ByteArray, Hash, HashElement, Int32, Simple, Type,
    VectorUInt64)
from .schema import Configurable


class ArraySchema(Configurable):
    data = ByteArray(
        displayedName="Data",
        description="The data of the array as an untyped buffer of bytes",
        accessMode=AccessMode.READONLY)

    shape = VectorUInt64(
        displayedName="Shape",
        description="The shape of the array reflects total dimensionality and "
                    "each element the extension in its dimension "
                    "(0: any extension)",
        accessMode=AccessMode.READONLY)

    type = Int32(
        displayedName="Data Type",
        description="The type of the contained array data",
        accessMode=AccessMode.READONLY)

    isBigEndian = Bool(
        displayedName="Is big-endian",
        description="A boolean flag which is true if the data is big-endian",
        accessMode=AccessMode.READONLY)


class NDArray(Type):
    """The NDArray Class

    The NDArray class is intended to store any multidimensional data occurring
    in Karabo. Internally it holds the data in a ByteArray.
    """
    _hashname = "NDARRAY"

    def __init__(self, *, node=None, dtype=None, shape=(), **kwargs):
        if node is not None:
            dtype = Type.types[node["type", "defaultValue"]].numpy
            shape = node["shape", "defaultValue"]
            kwargs['unitSymbol'] = node["data", "unitSymbol"]
            kwargs['metricPrefixSymbol'] = node["data", "metricPrefixSymbol"]
        if isinstance(dtype, type) and issubclass(dtype, Simple):
            dtype = dtype.numpy
        self.dtype = numpy.dtype(dtype)
        self.shape = shape
        super(NDArray, self).__init__(**kwargs)

    def toSchemaAndAttrs(self, device, state):
        _, attrs = super().toSchemaAndAttrs(device, state)
        attrs["classId"] = "NDArray"
        attrs["displayType"] = "NDArray"
        attrs["nodeType"] = NodeType.Node

        # Get the schema from the Configurable and declare default
        schema = ArraySchema.getClassSchema(device, state).hash
        schema["shape", "defaultValue"] = numpy.array(self.shape)
        schema["type", "defaultValue"] = self._gettype(self.dtype)
        schema["isBigEndian", "defaultValue"] = self.dtype.str[0] == ">"

        return schema, attrs

    def toKaraboValue(self, data, strict=False):
        if isinstance(data, Hash) and not strict:
            if data.empty() or len(data["data"]) == 0:
                return NoneValue(descriptor=self)
            dtype = numpy.dtype(Type.types[data["type"]].numpy)
            dtype = dtype.newbyteorder(
                ">" if data["isBigEndian"] else "<")
            ar = numpy.frombuffer(data["data"], count=data["shape"].prod(),
                                  dtype=dtype)
            ar.shape = data["shape"]
            return QuantityValue(ar, descriptor=self)

        if not isinstance(data, numpy.ndarray) or data.dtype != self.dtype:
            data = numpy.array(data, dtype=self.dtype)
        data = QuantityValue(data, descriptor=self)
        if data.units != self.units:
            data = data.to(self.units)
            data.descriptor = self

        return data

    def _gettype(self, dtype):
        dstr = dtype.str
        if dstr not in Type.strs:
            dstr = dtype.newbyteorder().str

        return Type.strs[dstr].number

    def toDataAndAttrs(self, data):
        attrs = {}
        if data.timestamp is not None:
            attrs = data.timestamp.toDict()

        # We are using fast-path Hash setting of values and attrs since
        # we don't have nodes in our Hash.
        h = Hash()
        h._setelement("type",
                      HashElement(self._gettype(data.dtype), attrs))
        h._setelement("isBigEndian",
                      HashElement(data.dtype.str[0] == ">", attrs))
        h._setelement("shape",
                      HashElement(numpy.array(data.shape), attrs))
        h._setelement("data",
                      HashElement(data.value.data, attrs))

        return h, attrs
