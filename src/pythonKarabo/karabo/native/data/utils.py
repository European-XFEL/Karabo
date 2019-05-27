import numpy as np

from .hash import Hash, Type


def dtype_from_number(number):
    """Return the corresponding dtype matching the Karabo Types to
    numpy dtypes

    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(Type.types[number].numpy)


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
