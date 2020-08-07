import numpy as np
from xml.sax.saxutils import escape

from .hash import Hash, HashList, Type


def dtype_from_number(number):
    """Return the dtype object matching the Karabo Types number

    in case of missing numpy definition in `Types`, returns an object_.
    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(numpyclass_from_number(number))


def numpyclass_from_number(number):
    """Return the dtype class matching the Karabo Types number

    in case of missing numpy definition in `Types`, returns an object_.
    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return getattr(Type.types[number], "numpy", np.object_)


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
    """Create the HTML representation of a Hash
    """
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
