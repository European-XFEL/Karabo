import numpy as np
from xml.sax.saxutils import escape

from .typenums import HashType
from .hash import Hash, HashList

__all__ = ['create_html_hash', 'dictToHash', 'dtype_from_number',
           'numpy_from_number']


def dtype_from_number(number):
    """Return the dtype object matching the Karabo Types number

    in case of missing numpy definition in `Types`, returns an object_.
    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(numpy_from_number(number))


def numpy_from_number(number, default=np.object_):
    """Return the numpy dtype class matching the Karabo Types number

    In case of missing numpy definition, returns an the `default`.
    >> numpy_from_number(16)
    >> numpy.int64
    """

    _TYPE = {
        HashType.VectorBool: np.bool_,
        HashType.VectorInt8: np.int8,
        HashType.VectorUInt8: np.uint8,
        HashType.VectorInt16: np.int16,
        HashType.VectorUInt8: np.uint16,
        HashType.VectorInt32: np.int32,
        HashType.VectorUInt32: np.uint32,
        HashType.VectorInt64: np.int64,
        HashType.VectorUInt64: np.uint64,
        HashType.VectorFloat: np.float32,
        HashType.VectorDouble: np.float64,
        HashType.VectorComplexFloat: np.complex64,
        HashType.VectorComplexDouble: np.complex128,

        HashType.Bool: np.bool_,
        HashType.Int8: np.int8,
        HashType.UInt8: np.uint8,
        HashType.Int16: np.int16,
        HashType.UInt16: np.uint16,
        HashType.Int32: np.int32,
        HashType.UInt32: np.uint32,
        HashType.Int64: np.int64,
        HashType.UInt64: np.uint64,
        HashType.Float: np.float32,
        HashType.Double: np.float64,
        HashType.ComplexFloat: np.complex64,
        HashType.ComplexDouble: np.complex128,
    }
    try:
        h_type = HashType(number)
    except Exception:
        return default

    return _TYPE.get(h_type, default)


def get_image_data(data):
    """Method to extract an ``image`` from a Hash into a numpy array

    This is a common use case when processing pipeline data (receiving from an
    output channel)

    :param data: A hash containing the image data hash
    :returns : A numpy array containing the extracted pixel data
    """
    text = "Expected a Hash, but got type %s instead!" % type(data)
    assert isinstance(data, Hash), text

    def extract_data(hsh):
        hsh = hsh["pixels"]
        dtype = dtype_from_number(hsh["type"])
        dtype = dtype.newbyteorder(">" if hsh["isBigEndian"] else "<")
        array = np.frombuffer(hsh["data"], count=hsh["shape"].prod(),
                              dtype=dtype)
        array.shape = hsh["shape"]
        return array

    if "data.image" in data:
        return extract_data(data["data.image"])

    elif "image" in data:
        return extract_data(data["image"])


def create_html_hash(hsh):
    """Create the HTML representation of a Hash"""
    assert isinstance(hsh, Hash), "An input of type ``Hash`` is required!"

    def _html_attributes(nest, attrs):
        for key, value in attrs.items():
            yield ('<tr><td style="padding-left:{}em">'
                   '<font size="1" color="red">'
                   '{}</td><td>'.format(nest + 1, key))
            yield '<font size="1" color="red">{}'.format(escape(str(value)))

    def _html_hash_generator(hsh, nest=0):
        if nest == 0:
            yield "<table>"
        for key, value, attr in hsh.iterall():
            if isinstance(value, Hash):
                yield ('<tr><td style="padding-left:{}em"><b>{}</b></td>'
                       '<td/></tr>'.format(nest + 1, key))
                yield from _html_hash_generator(value, nest + 1)
            elif isinstance(value, HashList):
                # XXX: Table support!
                continue
            else:
                yield ('<tr><td style="padding-left:{}em">{}</td><td>'
                       .format(nest + 1, key))
                yield escape(str(value))
                yield from _html_attributes(nest + 1, attr)
                yield '</td></tr>'
        if nest == 0:
            yield "</table>"

    return "".join(_html_hash_generator(hsh))


def dictToHash(d):
    """Convert a nested dictionary into a `Hash`"""
    h = Hash()
    for k, v in d.items():
        if isinstance(v, dict):
            h[k] = dictToHash(v)
        elif isinstance(v, (list, tuple)):
            if len(v) > 0 and isinstance(v[0], dict):
                # Note: This is a VectorHash
                h[k] = HashList([dictToHash(vv) for vv in v])
            else:
                h[k] = v
        else:
            h[k] = v
    return h
