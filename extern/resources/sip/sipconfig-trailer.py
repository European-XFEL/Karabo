# this is added by Karabo to make sip relocatable
try:
    newpath = __file__.rsplit("/", 1)[0]
    oldpath = _pkg_config["default_mod_dir"]

    for i, (l, r) in enumerate(zip(newpath[::-1], oldpath[::-1])):
        if l != r:
            break

    oldpath = oldpath[:-i]
    newpath = newpath[:-i]

    for key in list(_pkg_config):
        if key.endswith("_dir"):
            path = _pkg_config[key]
            assert path.startswith(oldpath)
            _pkg_config[key] = newpath + path[len(oldpath):]
except NameError:
    pass
