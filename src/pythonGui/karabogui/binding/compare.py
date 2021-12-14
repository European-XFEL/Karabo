from karabo.common import const
from karabo.native import Hash, is_equal, simple_deepcopy


def attr_fast_deepcopy(d, ref=None):
    """copy.deepcopy is criminally slow. We can bypass its fanciness as long
    as we only copy 'simple' datastructures.

    Pass a not None attributes dict to `ref` to get only changed attributes
    """
    out = {}

    for k, v in d.items():
        if ref is not None:
            if (k not in const.KARABO_EDITABLE_ATTRIBUTES or
                    is_equal(v, ref.get(k))):
                continue
        out[k] = simple_deepcopy(v)

    return out


def realign_hash(hsh, reference):
    """Reorders and fills the Hash `hsh` if necessary with respect to the
    key order `sequence`.

    - The Hash is build up according to `reference` and filled (`None`) if
     if the respective key is not in `hsh`.
    - Afterwards, if a key of the Hash `hsh` is not in the reference, it is
    appended at the end and assigned a `None` value.

    :param hsh: The Hash object
    :param reference: a sequence (list) of string keys as reference

    :return (Hash): new realigned `Hash`
    """

    ret = Hash()
    # 1. First iterate over the reference list to build up the new Hash
    for old_key in reference:
        if old_key in hsh:
            value, attrs = hsh.getElement(old_key)
            ret.setElement(old_key, simple_deepcopy(value),
                           simple_deepcopy(attrs))
        else:
            ret.setElement(old_key, None, {})

    # 2. Then, refill the hash keys that might not be in reference and append
    for key, value, attrs in Hash.flat_iterall(hsh):
        if key in reference:
            # Already considered
            continue

        ret.setElement(key, simple_deepcopy(value), simple_deepcopy(attrs))

    return ret
