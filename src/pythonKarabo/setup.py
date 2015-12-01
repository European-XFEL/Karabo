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
        from karabo._version import full_version as full_v
    except ImportError:
        raise ImportError("Unable to import vcs_revision. Try removing "
                          "karabo/_version.py and the build directory "
                          "before building.")

    match = re.match(r'.*?\.dev(?P<dev_num>\d+)', full_v)
    if match is None:
        dev_num = '0'
    else:
        dev_num = match.group('dev_num')

    return vcs_rev, dev_num


def _write_version_py(filename='karabo/_version.py'):
    from karabo.packaging.versioning import git_version, svn_version

    template = """\
# THIS FILE IS GENERATED FROM SETUP.PY
version = '{version}'
full_version = '{full_version}'
vcs_revision = '{vcs_revision}'
is_released = {is_released}

if not is_released:
    version = full_version
"""
    fullversion = VERSION
    vcs_rev, dev_num = 'Unknown', '0'

    # Try many ways to get version info
    path = op.normpath(op.dirname(__file__))
    version_generators = (
        partial(svn_version, path),
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
                                 is_released=IS_RELEASED))

if __name__ == '__main__':
    _write_version_py()
    from karabo.packaging.versioning import get_karabo_version

    setup(
        name="karabo",
        version=get_karabo_version(),
        author="Karabo Team",
        author_email="karabo@xfel.eu",
        description="This is the Python interface of the Karabo control system",
        url="http://karabo.eu",
        packages=find_packages(),
        package_data={
            'karabo.api1.tests': ['resources/*.*'],
            'karabo.api2.tests': ['*.xml'],
        },
        entry_points={'console_scripts': [
                      'karabo_device_server=karabo.api1.device_server:main',
                      'ideviceclient=karabo.interactive.ideviceclient:main',
                      'ikarabo=karabo.interactive.ikarabo:main',
                      'convert-karabo-device-project=karabo.interactive.convert_device_project:main',
                      ]},
    )
