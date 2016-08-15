import distutils.command.build
from functools import partial
import json
import os.path as op
import sys

from setuptools import setup, find_packages

MAJOR = 1
MINOR = 5
MICRO = 0
IS_RELEASED = False
VERSION = '%d.%d.%d' % (MAJOR, MINOR, MICRO)
VERSION_FILE_PATH = op.join('karabo_gui', '_version.py')


def _get_src_dist_version():
    if not op.exists(VERSION_FILE_PATH):
        return 'Unknown', '0'

    # must be a source distribution, use existing version file
    try:
        from karabo_gui._version import vcs_revision as vcs_rev
        from karabo_gui._version import vcs_revision_count as dev_num
    except ImportError:
        msg = ("Unable to import vcs_revision. Try removing {} and the build "
               "directory before building.").format(VERSION_FILE_PATH)
        raise ImportError(msg)

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
    metadata = {
        'name': 'KaraboGUI',
        'version': version,
        'author': 'Karabo Team',
        'author_email': 'karabo@xfel.eu',
        'description': 'This is the Karabo GUI',
        'url': 'http://karabo.eu',
        'packages': [PKG] + [PKG + '.' + pkg for pkg in find_packages(PKG)],
        'package_data': {
            "karabo_gui.dialogs": ["*.ui"],
            "karabo_gui.displaywidgets": ["*.ui", "*.svg"],
            "karabo_gui.icons": ["*.*", "vacuum/*.*",
                                 "vacuum/bigger/*.*"],
            "karabo_gui.tests": ["*.xml", "project/*.xml",
                                 "project/devices/*.xml",
                                 "project/resources/icon/*.png",
                                 "project/scenes/*.svg"],
        }
    }

    WINDOWS_BUILDER = 'krb_windows_build'
    if WINDOWS_BUILDER in sys.argv:
        # Add a subset of the Karabo package for the Windows build
        metadata['package_dir'] = {'karabo': "../../src/pythonKarabo/karabo"}
        metadata['packages'].extend(
            ["karabo", "karabo.middlelayer_api",
             "karabo.middlelayer_api._project", "karabo.interactive",
             "karabo.packaging", "karabo.testing"]
        )
        # Write out useful data
        with open('VERSION', 'w') as fp:
            fp.write(version + '\n')
        with open('METADATA', 'w') as fp:
            json.dump(metadata, fp)

    setup(entry_points={'console_scripts': [
                        'karabo-gui=karabo_gui.main:main',
                        'scene-runner=karabo_gui.sceneview.runner:main',
                        ]},
          # Add an alias for 'build' so we can prepare data for Windows
          cmdclass={WINDOWS_BUILDER: distutils.command.build.build},
          **metadata)
