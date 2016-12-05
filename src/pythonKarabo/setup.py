from functools import partial
import os.path as op
import re
import subprocess

from setuptools import setup, find_packages

MAJOR = 2
MINOR = 0
MICRO = 3
IS_RELEASED = False
VERSION = '%d.%d.%d' % (MAJOR, MINOR, MICRO)
VERSION_FILE_PATH = op.join('karabo', '_version.py')


def _get_version(path):
    """ Returns the contents of a version tag if it matches the
    MAJOR.MINOR.MICRO format.
    """
    global IS_RELEASED
    try:
        cmd = 'git describe --exact-match HEAD'.split(' ')
        out = subprocess.check_output(cmd, cwd=path).strip().decode('ascii')
        IS_RELEASED = True
    except (subprocess.CalledProcessError, OSError):
        out = VERSION

    # Make sure this is a version tag
    expr = r'(\d+)\.(\d+)\.(\d+)$'
    match = re.match(expr, out)
    if match is None:
        out = VERSION  # Use hard-coded version instead
        IS_RELEASED = False

    return out


def _get_src_dist_version():
    if not op.exists(VERSION_FILE_PATH):
        return 'Unknown', '0'

    # must be a source distribution, use existing version file
    try:
        from karabo._version import vcs_revision as vcs_rev
        from karabo._version import vcs_revision_count as dev_num
    except ImportError:
        raise ImportError("Unable to import vcs_revision. Try removing "
                          "{} and the build directory "
                          "before building.").format(VERSION_FILE_PATH)

    return vcs_rev, dev_num


def _write_version_py(filename=VERSION_FILE_PATH):
    from karabo.packaging.versioning import git_version

    template = """\
# THIS FILE IS GENERATED FROM SETUP.PY
version = '{version}'
full_version = '{full_version}'
vcs_revision = '{vcs_revision}'
vcs_revision_count = '{vcs_revision_count}'
is_released = {is_released}
"""
    path = op.normpath(op.dirname(__file__))

    version = _get_version(path)
    fullversion = version
    vcs_rev, dev_num = 'Unknown', '0'

    # Try many ways to get version info
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
        fp.write(template.format(version=version,
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
            'karabo.bound_api.tests': ['resources/*.*'],
            "karabo.common.scenemodel.tests": ["data/*.svg",
                                               "data/inkscape/*.svg",
                                               "data/legacy/*.svg",
                                               "data/legacy/icon_data/*.svg"],
            'karabo.middlelayer_api.tests': ['*.xml'],
            'karabo.testing': ['resources/*.*'],
            'karabo.project_db': ['config_stubs/*.*'],
            'karabo.integration_tests': ['device_comm_test/CommTestDevice.egg-info/*.*'],
        },
        entry_points={
            'console_scripts': [
                'karabo=karabo.interactive.karabo:main',
                'karabo-pythonserver=karabo.bound_api.device_server:main',
                'karabo-middlelayerserver=karabo.middlelayer_api.device_server:main',
                'karabo-cli=karabo.interactive.ideviceclient:main',
                'ikarabo=karabo.interactive.ikarabo:main',
                'convert-karabo-device=karabo.interactive.convert_device_project:main',
                'convert-karabo-project=karabo.interactive.upgrade_project_data:main',
             ],
            'karabo.bound_device': [
                'ProjectManager=karabo.bound_devices.project_manager:ProjectManager',
            ],
            'karabo.middlelayer_device': [
                'IPythonKernel=karabo.middlelayer_api.ipython:IPythonKernel',
                'MetaMacro=karabo.middlelayer_api.metamacro:MetaMacro'
            ]
        },
    )
