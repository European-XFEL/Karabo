# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from karabo.native import (
    Bool, ByteArray, Configurable, Encoding, Hash, Int8, Node, VectorInt8,
    VectorInt16)

# Reverse map of controllers.images.
TYPENUM_MAP = {
    'bool': 0,
    'char': 2,
    'int8': 4,
    'uint8': 6,
    'int16': 8,
    'uint16': 10,
    'int32': 12,
    'uint32': 14,
    'int64': 16,
    'uint64': 18,
    'float': 20,
    'float64': 22,
}


class Pixels(Configurable):
    type = Int8()
    data = ByteArray()


class Alignment(Configurable):
    offsets = VectorInt8()
    rotations = VectorInt8()


class Geometry(Configurable):
    update = Bool(False)
    pixelRegion = VectorInt8()
    alignment = Node(Alignment)


class ImageNode(Configurable):
    pixels = Node(Pixels)
    dims = VectorInt16()
    geometry = Node(Geometry)
    encoding = Int8()


class ImageNodeWithStackAxis(ImageNode):
    stackAxis = Int8()


def get_output_node(stack_axis=True):
    """Returns a node that contains an image node that could contain a
       stackAxis property"""
    image_node = ImageNodeWithStackAxis if stack_axis else ImageNode

    class OutputNodeInner(Configurable):
        image = Node(image_node, displayType='Image')

    class OutputNode(Configurable):
        data = Node(OutputNodeInner)

    return OutputNode


def get_pipeline_schema(stack_axis=True):
    """Returns a schema that contains an output node. This node has an image
       node that could contain a stackAxis property"""
    class PipelineData(Configurable):
        output = Node(get_output_node(stack_axis), displayType='OutputChannel')

    return PipelineData().getDeviceSchema()


def _get_geometry_hash(update):
    alignment_hash = Hash('offsets', [0., 0., 0.],
                          'rotations', [0., 0., 0.])
    return Hash('update', update,
                'pixelRegion', [0, 0, 1, 1],  # x0, y0, x1, y1
                'alignment', alignment_hash)


ZAXIS = 2
dimX = 40
dimY = 30


def get_image_hash(val=0, dimZ=None, *, encoding=Encoding.GRAY,
                   stack_axis=True, update=True):
    npix = dimX * dimY
    dims_val = [dimY, dimX]
    if dimZ:
        dims_val.append(dimZ)
        npix *= dimZ
    pixel_hsh = Hash('type', TYPENUM_MAP['uint8'],
                     'data', bytearray([val for _ in range(npix)]))
    img_hsh = Hash('pixels', pixel_hsh,
                   'dims', dims_val,
                   'geometry', _get_geometry_hash(update),
                   'encoding', encoding)
    if stack_axis:
        img_hsh.set('stackAxis', ZAXIS)
    return Hash('image', img_hsh)
