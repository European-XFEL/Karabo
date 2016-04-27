from functools import partial
import os.path as op

from setuptools import setup, find_packages

MAJOR = 1
MINOR = 5
MICRO = 0
IS_RELEASED = False
VERSION = '%d.%d.%d' % (MAJOR, MINOR, MICRO)
VERSION_FILE_PATH = 'karabo_gui/_version.py'


def _get_src_dist_version():
    if not op.exists(VERSION_FILE_PATH):
        return 'Unknown', '0'

    # must be a source distribution, use existing version file
    try:
        from karabo_gui._version import vcs_revision as vcs_rev
        from karabo_gui._version import vcs_revision_count as dev_num
    except ImportError:
        raise ImportError("Unable to import vcs_revision. Try removing "
                          + VERSION_FILE_PATH + " and the build directory "
                          "before building.")

    return vcs_rev, dev_num


def _write_version_py(filename=VERSION_FILE_PATH):
    from karabo.packaging.versioning import git_version

    template = """\
# THIS FILE IS GENERATED FROM SETUP.PY. DO NOT EDIT.
version = '{version}'
full_version = '{full_version}'
vcs_revision = '{vcs_revision}'
vcs_revision_count = '{vcs_revision_count}'
is_released = {is_released}
"""
    fullversion = VERSION
    vcs_rev, dev_num = 'Unknown', '0'

    # Try many ways to get version info
    path = op.normpath(op.dirname(__file__))
    version_generators = (
        partial(git_version, path),
        _get_src_dist_version,
    )
    for version_gen in version_generators:
        vcs_rev, dev_num = version_gen()
        if vcs_rev != 'Unknown':
            break

    if not IS_RELEASED:
        fullversion += '.dev{0}'.format(dev_num)

    with open(filename, "wt") as fp:
        fp.write(template.format(version=VERSION,
                                 full_version=fullversion,
                                 vcs_revision=vcs_rev,
                                 vcs_revision_count=dev_num,
                                 is_released=IS_RELEASED))

    return fullversion

if __name__ == '__main__':
    version = _write_version_py()

    PKG = 'karabo_gui'
    setup(name=PKG,
          version=version,
          author="Karabo Team",
          author_email="karabo@xfel.eu",
          description="This is the Karabo GUI",
          url="http://karabo.eu",
          packages=[PKG] + [PKG + '.' + pkg for pkg in find_packages(PKG)],
          package_data = {"karabo_gui.icons": ["*.*", "vacuum/*.*",
                                               "vacuum/bigger/*.*"],
                          "karabo_gui.displaywidgets": ["*.ui", "*.svg"],
                          "karabo_gui.dialogs": ["*.ui"]
                          },
          entry_points={'console_scripts': [
                        'karabo-gui=karabo_gui.main:main',
                        ]},
    )
