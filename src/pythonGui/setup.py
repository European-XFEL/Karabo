import distutils.command.build
import json
import os.path as op
import sys

from setuptools import setup, find_packages

VERSION_FILE_PATH = op.join('karabogui', '_version.py')


def _get_src_dist_version():
    if not op.exists(VERSION_FILE_PATH):
        return 'Unknown', '0', '0.0.0'

    # must be a source distribution, use existing version file
    try:
        from karabogui._version import full_version as version
        from karabogui._version import vcs_revision as vcs_rev
        from karabogui._version import vcs_revision_count as dev_num
    except ImportError:
        msg = ("Unable to import vcs_revision. Try removing {} and the build "
               "directory before building.").format(VERSION_FILE_PATH)
        raise ImportError(msg)

    return vcs_rev, dev_num, version


def _get_version(path):
    vcs_rev, dev_num, version = _get_src_dist_version()

    if vcs_rev != 'Unknown':
        return {'vcs_revision': vcs_rev, 'revision_count': dev_num,
                'version': version, 'released': True}

    from karabo.packaging.versioning import git_version
    return git_version(path)


def _write_version_py(filename=VERSION_FILE_PATH):
    template = """\
# THIS FILE IS GENERATED FROM SETUP.PY
version = '{version}'
full_version = '{full_version}'
vcs_revision = '{vcs_revision}'
vcs_revision_count = '{vcs_revision_count}'
is_released = {is_released}
"""
    path = op.normpath(op.dirname(__file__))

    vers_dict = _get_version(path)
    version = vers_dict['version']
    fullversion = version

    if not vers_dict['released']:
        fullversion += '.dev{0}'.format(vers_dict['revision_count'])

    if not op.exists(filename):
        with open(filename, "wt") as fp:
            fp.write(template.format(
                version=version, full_version=fullversion,
                vcs_revision=vers_dict['vcs_revision'],
                vcs_revision_count=vers_dict['revision_count'],
                is_released=vers_dict['released']))

    return fullversion


if __name__ == '__main__':
    version = _write_version_py()

    metadata = {
        'name': 'KaraboGUI',
        'version': version,
        'author': 'Karabo Team',
        'author_email': 'karabo@xfel.eu',
        'description': 'This is the Karabo GUI',
        'url': 'http://karabo.eu',
        'packages': find_packages(),
        'include_package_data': True
    }

    WINDOWS_BUILDER = 'krb_windows_build'
    if WINDOWS_BUILDER in sys.argv:
        # Add a subset of the Karabo package for the Windows build
        metadata['package_dir'] = {'karabo': "../../src/pythonKarabo/karabo"}
        metadata['packages'].extend(
            ["karabo", "karabo.common", "karabo.common.scenemodel",
             "karabo.common.scenemodel.widgets", "karabo.common.project",
             "karabo.interactive", "karabo.middlelayer_api",
             "karabo.middlelayer_api.project", "karabo.packaging",
             "karabo.testing"]
        )
        # Write out useful data
        with open('VERSION', 'w') as fp:
            fp.write(version + '\n')
        with open('METADATA', 'w') as fp:
            json.dump(metadata, fp)

    setup(entry_points={'console_scripts': [
                'karabo-gui=karabogui.programs.gui_runner:main',
                'panel-runner=karabogui.programs.panel_runner:main',
                'karabo-cinema=karabogui.programs.cinema:main',
    ]},
          # Add an alias for 'build' so we can prepare data for Windows
          cmdclass={WINDOWS_BUILDER: distutils.command.build.build},
          **metadata)
