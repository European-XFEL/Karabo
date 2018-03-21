from karabo.middlelayer_api.hash import ByteArray
from karabo.middlelayer import (
    Bool, Configurable, EncodingType, Hash, Int8, Node,
    VectorInt8, VectorInt16)


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
    stackAxis = Int8()
    geometry = Node(Geometry)
    encoding = Int8()


class OutputNodeInner(Configurable):
    image = Node(ImageNode, displayType='Image')


class OutputNode(Configurable):
    data = Node(OutputNodeInner)


class PipelineData(Configurable):
    output = Node(OutputNode, displayType='OutputChannel')


def _get_geometry_hash(update):
    alignment_hash = Hash('offsets', [0., 0., 0.],
                          'rotations', [0., 0., 0.])
    return Hash('update', update,
                'pixelRegion', [0, 0, 1, 1],  # x0, y0, x1, y1
                'alignment', alignment_hash)


ZAXIS = 2
dimX = 40
dimY = 30


def get_image_hash(val=0, dimZ=None, *, encoding=EncodingType.GRAY,
                   update=True):
    npix = dimX * dimY
    dims_val = [dimY, dimX]
    if dimZ:
        dims_val.append(dimZ)
        npix *= dimZ
    pixel_hsh = Hash('type', TYPENUM_MAP['uint8'],
                     'data', bytearray([val for _ in range(npix)]))
    img_hsh = Hash('pixels', pixel_hsh,
                   'dims', dims_val,
                   'stackAxis', ZAXIS,
                   'geometry', _get_geometry_hash(update),
                   'encoding', encoding)
    return Hash('image', img_hsh)
