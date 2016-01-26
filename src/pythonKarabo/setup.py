from functools import partial
import os.path as op
import re

from setuptools import setup, find_packages

MAJOR = 1
MINOR = 5
MICRO = 0
IS_RELEASED = False
VERSION = '%d.%d.%d' % (MAJOR, MINOR, MICRO)


def _get_src_dist_version():
    if not op.exists('karabo/_version.py'):
        return 'Unknown', '0'

    # must be a source distribution, use existing version file
    try:
        from karabo._version import vcs_revision as vcs_rev
        from karabo._version import vcs_revision_count as dev_num
    except ImportError:
        raise ImportError("Unable to import vcs_revision. Try removing "
                          "karabo/_version.py and the build directory "
                          "before building.")

    return vcs_rev, dev_num


def _write_version_py(filename='karabo/_version.py'):
    from karabo.packaging.versioning import (git_version, svn_version,
                                             jsvn_version)

    template = """\
# THIS FILE IS GENERATED FROM SETUP.PY
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
        partial(svn_version, path),
        partial(jsvn_version, path),
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

    setup(
        name="karabo",
        version=version,
        author="Karabo Team",
        author_email="karabo@xfel.eu",
        description="This is the Python interface of the Karabo control system",
        url="http://karabo.eu",
        packages=find_packages(),
        package_data={
            'karabo.api1.tests': ['resources/*.*'],
            'karabo.api2.tests': ['*.xml'],
            'karabo.testing': ['resources/*.*'],
        },
        entry_points={'console_scripts': [
                      'karabo_device_server=karabo.api1.device_server:main',
                      'ideviceclient=karabo.interactive.ideviceclient:main',
                      'ikarabo=karabo.interactive.ikarabo:main',
                      'convert-karabo-device-project=karabo.interactive.convert_device_project:main',
                      'generate-karabo-project=karabo.interactive.project_generator:main',
                      ]},
    )
