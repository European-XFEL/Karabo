# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import numpy

from karabo.native.data import (
    AccessMode, DaqDataType, EncodingType, Hash, NodeType, Unit,
    dtype_from_number)

from .basetypes import ImageData, NoneValue
from .configurable import Configurable
from .descriptors import (
    Bool, Int32, Simple, String, Type, VectorInt32, VectorUInt64)
from .ndarray import NDArray


def convert_dtype(dtype):
    dstr = dtype.str
    if dstr not in Type.strs:
        dstr = dtype.newbyteorder().str

    return Type.strs[dstr].number


class ImageNode(Configurable):
    pixels = NDArray(
        displayedName="Pixels",
        description="The N-dimensional array containing the pixels",
        accessMode=AccessMode.READONLY)

    dims = VectorUInt64(
        displayedName="Dimensions",
        description="The length of the array reflects total dimensionality "
                    "and each element the extension in this dimension",
        maxSize=8,
        accessMode=AccessMode.READONLY)

    dimTypes = VectorInt32(
        displayedName="Dimension Types",
        description="Any dimension should have an enumerated type",
        maxSize=8,
        accessMode=AccessMode.READONLY)

    dimScales = String(
        displayedName="Dimension Scales",
        description="",
        accessMode=AccessMode.READONLY)

    encoding = Int32(
        displayedName="Encoding",
        description="Describes the color space of pixel encoding of the data"
                    "(e.g. GRAY, RGB, JPG, PNG etc.",
        accessMode=AccessMode.READONLY)

    bitsPerPixel = Int32(
        displayedName="Bits per pixel",
        description="The number of bits needed for each pixel",
        accessMode=AccessMode.READONLY)

    roiOffsets = VectorUInt64(
        displayedName="ROI Offsets",
        description="Describes the offset of the Region-of-Interest. It will"
                    "contain zeros if the image has no ROI defined",
        maxSize=8,
        accessMode=AccessMode.READONLY)

    binning = VectorUInt64(
        displayedName="Binning",
        description="The number of binned adjacent pixels. They "
                    "are reported out of the camera as a single pixel.",
        maxSize=8,
        accessMode=AccessMode.READONLY)

    rotation = Int32(
        displayedName="Rotation",
        description="The image counterclockwise rotation.",
        options=[0, 90, 180, 270],
        unitSymbol=Unit.DEGREE,
        accessMode=AccessMode.READONLY)

    flipX = Bool(
        displayedName="Flip X",
        description="Image horizontal flip.",
        accessMode=AccessMode.READONLY)

    flipY = Bool(
        displayedName="Flip Y",
        description="Image vertical flip.",
        accessMode=AccessMode.READONLY)


class Image(Type):
    """The `Image` class is a helper class to provide ImageData

    Along the raw pixel values it also stores useful metadata like encoding,
    bit depth or binning and basic transformations like flip, rotation, ROI.

    This special hash `Type` contains an NDArray element and is constructed::

        class Device(Configurable):

            data = ImageData(np.zeros(shape=(10, 10), dtype=np.uint64),
                             encoding=ENCODING.GRAY)
            image = Image(
                data=data,
                displayedName="Image")

    Hence, the `Image` element can be initialized with an `ImageData`
    KaraboValue.

    Alternatively, the `Image` element can be initialized by providing `shape`
    and `dtype` and the `encoding`::

        image = Image(
            displayedName="Image"
            shape=(2600, 2000),
            dtype=UInt8,
            encoding=EncodingType.GRAY)

    The `dtype` can be provided with a simple Karabo descriptor or the numpy
    dtype, e.g. numpy.uint8.
    """

    _hashname = "HASH"
    defaultValue = Hash()

    def __init__(self, data=None, dtype=None, shape=None,
                 encoding=EncodingType.GRAY, daqDataType=DaqDataType.TRAIN,
                 **kwargs):
        if dtype is not None and shape is not None:
            if isinstance(dtype, type) and issubclass(dtype, Simple):
                dtype = dtype.numpy
            data = ImageData(numpy.zeros(shape=shape, dtype=dtype),
                             encoding=encoding)
        if data is not None and not isinstance(data, ImageData):
            raise TypeError("The `Image` element requires an `ImageData` "
                            "object for initialization, but got {} "
                            "instead".format(type(data)))

        self.__dict__.update(data.toDict())
        self.daqDataType = daqDataType
        super().__init__(accessMode=AccessMode.READONLY, **kwargs)

    def toSchemaAndAttrs(self, device, state):
        _, attrs = super().toSchemaAndAttrs(device, state)
        attrs["classId"] = "ImageData"
        attrs["displayType"] = "ImageData"
        attrs["nodeType"] = NodeType.Node
        attrs["daqDataType"] = self.daqDataType.value

        # Get the schema from the Configurable and declare default
        schema = ImageNode.getClassSchema(device, state).hash
        schema["pixels.shape", "defaultValue"] = numpy.array(
            self.shape, dtype=numpy.uint64)
        schema["pixels.type", "defaultValue"] = convert_dtype(self.dtype)
        schema["pixels.isBigEndian", "defaultValue"] = self.dtype.str[0] == ">"

        # Set the attribute defaultValues!
        schema["dims", "defaultValue"] = self.dims
        schema["dimTypes", "defaultValue"] = self.dimTypes
        schema["dimScales", "defaultValue"] = self.dimScales
        schema["encoding", "defaultValue"] = self.encoding
        schema["bitsPerPixel", "defaultValue"] = self.bitsPerPixel
        schema["roiOffsets", "defaultValue"] = self.roiOffsets
        schema["binning", "defaultValue"] = self.binning
        schema["rotation", "defaultValue"] = self.rotation
        schema["flipX", "defaultValue"] = self.flipX
        schema["flipY", "defaultValue"] = self.flipY

        # Set the attribute maxSizes!
        maxSize = numpy.uint32(len(self.shape))
        schema["pixels.shape", "maxSize"] = maxSize
        schema["dims", "maxSize"] = maxSize
        schema["dimTypes", "maxSize"] = maxSize
        schema["roiOffsets", "maxSize"] = maxSize
        schema["binning", "maxSize"] = maxSize

        return schema, attrs

    def toKaraboValue(self, data, strict=False):
        if isinstance(data, Hash) and not strict:
            if (data.empty()
                    or "pixels" not in data
                    or len(data["pixels"]) == 0):
                return NoneValue(descriptor=self)

            pixels = data["pixels"]
            dtype = dtype_from_number(data["type"])
            dtype = dtype.newbyteorder(">" if pixels["isBigEndian"] else "<")
            ar = numpy.frombuffer(pixels["data"], count=pixels["shape"].prod(),
                                  dtype=dtype)
            ar.shape = pixels["shape"]
            return ImageData(value=ar,
                             dimTypes=self.dimTypes,
                             dimScales=self.dimScales,
                             encoding=self.encoding,
                             bitsPerPixel=self.bitsPerPixel,
                             roiOffsets=self.roiOffsets,
                             binning=self.binning,
                             rotation=self.rotation,
                             flipX=self.flipX,
                             flipY=self.flipY,
                             descriptor=self)

        if isinstance(data, ImageData):
            data.descriptor = self
            return data

        if not isinstance(data, numpy.ndarray) or data.dtype != self.dtype:
            data = numpy.array(data, dtype=self.dtype)

        # If only an array was provided, we set our predefined values
        if not isinstance(data, ImageData):
            data = ImageData(value=data,
                             dimTypes=self.dimTypes,
                             dimScales=self.dimScales,
                             encoding=self.encoding,
                             bitsPerPixel=self.bitsPerPixel,
                             roiOffsets=self.roiOffsets,
                             binning=self.binning,
                             rotation=self.rotation,
                             flipX=self.flipX,
                             flipY=self.flipY,
                             descriptor=self)
        return data

    def toDataAndAttrs(self, data):
        if not isinstance(data, ImageData):
            raise TypeError("The `Image` element requires an `ImageData` "
                            "object for initialization, but got {} "
                            "instead".format(type(data)))
        attrs = {}
        if data.timestamp is not None:
            attrs = data.timestamp.toDict()

        # Set the attributes of the image data!
        h = Hash()
        h.setElement("dims", data.dims, attrs)
        h.setElement("dimTypes", data.dimTypes, attrs)
        h.setElement("dimScales", data.dimScales, attrs)
        h.setElement("encoding", data.encoding, attrs)
        h.setElement("roiOffsets", data.roiOffsets, attrs)
        h.setElement("binning", data.binning, attrs)
        h.setElement("rotation", data.rotation, attrs)
        h.setElement("bitsPerPixel", data.bitsPerPixel, attrs)
        h.setElement("flipX", data.flipX, attrs)
        h.setElement("flipY", data.flipY, attrs)

        # Finally, set the NDArray Hash!
        pixels = Hash()
        pixels.setElement("type", convert_dtype(data.dtype), attrs)
        pixels.setElement("isBigEndian", data.dtype.str[0] == ">", attrs)
        pixels.setElement("shape", data.shape, attrs)
        pixels.setElement("data", data.value.data, attrs)

        # set the `__classId` attribute to allow the C++ API to decode the
        # `pixels` Hash as an NDArray object.
        # XXX: This is a code duplication of NDArray.toDataAndAttrs
        array_attrs = {"__classId": "NDArray"}
        array_attrs.update(**attrs)
        h.setElement("pixels", pixels, array_attrs)

        # set the `__classId` attribute to allow the C++ API to decode this
        # Hash node into an ImageData Object.
        image_attrs = {"__classId": "ImageData"}
        image_attrs.update(**attrs)
        return h, image_attrs
