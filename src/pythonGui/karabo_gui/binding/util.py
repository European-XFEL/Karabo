
def fast_deepcopy(d):
    """copy.deepcopy is criminally slow. We can bypass its fanciness as long
    as we only copy 'simple' datastructures.
    """
    out = dict.fromkeys(d)
    for k, v in d.items():
        try:
            out[k] = v.copy()  # dicts, sets, ndarrays
        except AttributeError:
            try:
                out[k] = v[:]  # lists, tuples, strings, unicode
            except TypeError:
                out[k] = v  # simple values

    return out
