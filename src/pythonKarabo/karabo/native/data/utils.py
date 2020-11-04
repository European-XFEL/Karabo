import numpy as np
from xml.sax.saxutils import escape

from .enums import NodeType
from .hash import Hash, HashList, Type

NODE_TYPE = (NodeType.Node.value,)
SPECIAL_TYPES = (NodeType.ChoiceOfNodes.value, NodeType.ListOfNodes.value)


def dtype_from_number(number):
    """Return the dtype object matching the Karabo Types number

    in case of missing numpy definition in `Types`, returns an object_.
    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(numpy_from_number(number))


def numpy_from_number(number, default=np.object_):
    """Return the numpy dtype class matching the Karabo Types number

    In case of missing numpy definition in `Types`, returns an the `default`.
    >> numpy_from_number(16)
    >> numpy.int64
    """
    return getattr(Type.types[number], "numpy", default)


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


def flat_iter_hash(config, base=''):
    """Recursively iterate over all the keys in a Hash object such that
    a simple iterator interface is exposed.
    """
    base = base + '.' if base else ''
    for key, value, _ in config.iterall():
        subkey = base + key
        if isinstance(value, Hash):
            yield from flat_iter_hash(value, base=subkey)
        else:
            yield subkey


def flat_iterall_hash(config, base=''):
    """Recursively iterate over all parameters in a Hash object such that
    a simple iterator interface is exposed.
    """
    base = base + '.' if base else ''
    for key, value, attrs in config.iterall():
        subkey = base + key
        if isinstance(value, Hash):
            yield from flat_iterall_hash(value, base=subkey)
        else:
            yield subkey, value, attrs


def flat_iter_schema_hash(schema_hash, base=''):
    """Expose a flat iteration over a schema Hash.

    :param schema: The schema Hash or Schema object

    Note: Schema Hashes are special because every property
    comes with an empty `Hash` as value. Hence, we ask for the Nodetype!
    """
    assert isinstance(schema_hash, Hash)

    base = base + '.' if base else ''
    for key, value, attrs in schema_hash.iterall():
        subkey = base + key
        is_node = attrs["nodeType"] in NODE_TYPE
        is_special = attrs["nodeType"] in SPECIAL_TYPES
        if is_node:
            yield from flat_iter_schema_hash(value, base=subkey)
        elif not is_special:
            yield subkey
