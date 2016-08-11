from types import MethodType
import weakref

import numpy as np

from karathon import (
    Hash, ImageDataBOOL, ImageDataINT8, ImageDataINT16, ImageDataINT32,
    ImageDataINT64, ImageDataUINT8, ImageDataUINT16, ImageDataUINT32,
    ImageDataUINT64, ImageDataFLOAT, ImageDataDOUBLE
)

_CLASS_MAP = {
    np.bool: ImageDataBOOL,
    np.uint8: ImageDataUINT8,
    np.uint16: ImageDataUINT16,
    np.uint32: ImageDataUINT32,
    np.uint64: ImageDataUINT64,
    np.int8: ImageDataINT8,
    np.int16: ImageDataINT16,
    np.int32: ImageDataINT32,
    np.int64: ImageDataINT64,
    np.float32: ImageDataFLOAT,
    np.float64: ImageDataDOUBLE
}


def _add_image_data_methods(img_data):
    """ Connect all the ImageData methods. These wrapper methods hold a
    weak reference to C++ ImageData<T> object so that they won't keep the
    object alive.
    """
    weak_obj_ref = weakref.ref(img_data._internal)

    def wrap_method(name):
        def method(self, *args, **kwargs):
            obj = weak_obj_ref()
            if obj is None:
                msg = "Tried to call method {} on a dead object"
                raise RuntimeError(msg.format(name))
            func = getattr(obj, name)
            return func(*args, **kwargs)

        orig_meth = getattr(img_data._internal, name)
        method.__doc__ = orig_meth.__doc__
        return MethodType(method, img_data)

    IMAGEDATA_METHOD_NAMES = (
        'get', 'getData', 'getDimensionScales', 'getDimensionTypes',
        'getDimensions', 'getEncoding', 'getGeometry', 'getHeader',
        'getROIOffsets', 'hash', 'setData', 'setDimensionScales',
        'setDimensionTypes', 'setDimensions', 'setEncoding', 'setGeometry',
        'setHeader', 'setIsBigEndian', 'setROIOffsets', 'toBigEndian',
        'toLittleEndian'
    )
    for name in IMAGEDATA_METHOD_NAMES:
        setattr(img_data, name, wrap_method(name))


class ImageData(object):
    """ An object for storing N-dimensional image data.
    """
    def __init__(self, data, **kwargs):
        if not isinstance(data, (np.ndarray, Hash)):
            raise ValueError("ImageData must be constructed with an ndarray")

        # Get the data type information from the 
        if isinstance(data, Hash):
            if 'data' not in data:
                msg = ("Hash for constructing an ImageData must contain a "
                       "numpy array named 'data'")
                raise ValueError(msg)
            dtype = data['data'].dtype
        else:
            dtype = data.dtype

        klass = _CLASS_MAP.get(dtype.type)
        if klass is None:
            msg = "ImageData does not work with arrays of type {}"
            raise ValueError(msg.format(dtype))

        self._internal = klass(data, **kwargs)
        _add_image_data_methods(self)
