import numpy

from .basetypes import NoneValue, QuantityValue
from .hash import Attribute, Descriptor, Hash, Simple, Type


class NDArray(Type):
    shape = Attribute(())

    def __init__(self, *, node=None, dtype=None, shape=(), **kwargs):
        if node is not None:
            dtype = Type.types[node["type", "defaultValue"]].numpy
            shape = node["shape", "defaultValue"]
            kwargs['unitSymbol'] = node["data", "unitSymbol"]
            kwargs['metricPrefixSymbol'] = node["data", "metricPrefixSymbol"]
        if isinstance(dtype, type) and issubclass(dtype, Simple):
            dtype = dtype.numpy
        self.dtype = numpy.dtype(dtype)
        super(NDArray, self).__init__(shape=shape, **kwargs)

    def toSchemaAndAttrs(self, device, state):
        schema, attrs = Descriptor.toSchemaAndAttrs(self, device, state)
        attrs["classId"] = "NDArray"
        attrs["displayType"] = "NDArray"
        attrs["shape"] = numpy.array(self.shape)
        attrs["type"] = self._gettype(self.dtype)
        return schema, attrs

    def toKaraboValue(self, data, strict=False):
        if isinstance(data, Hash) and not strict:
            if len(data["data"]) == 0:
                return NoneValue(descriptor=self)
            dtype = numpy.dtype(Type.types[data["type"]].numpy)
            dtype = dtype.newbyteorder(">" if data["isBigEndian"] else "<")
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
        dstr = self.dtype.str
        if dstr not in Type.strs:
            dstr = self.dtype.newbyteorder().str
        return Type.strs[dstr].number

    def toDataAndAttrs(self, data):
        attrs = {}
        if data.timestamp is not None:
            attrs = data.timestamp.toDict()
        h = Hash("type", self._gettype(data.dtype),
                 "isBigEndian", data.dtype.str[0] == ">",
                 "shape", numpy.array(data.shape),
                 "data", data.value.data)
        return h, attrs
