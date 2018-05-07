import os.path as op
import sys

from jupyter_client.kernelspec import install_kernel_spec
from setuptools import setup, find_packages
from setuptools.command.install import install

VERSION_FILE_PATH = op.join('karabo', '_version.py')


def _get_src_dist_version():
    if not op.exists(VERSION_FILE_PATH):
        return 'Unknown', '0', '0.0.0'

    # must be a source distribution, use existing version file
    try:
        from karabo._version import full_version as version
        from karabo._version import vcs_revision as vcs_rev
        from karabo._version import vcs_revision_count as dev_num
    except ImportError:
        raise ImportError("Unable to import vcs_revision. Try removing "
                          "{} and the build directory "
                          "before building.").format(VERSION_FILE_PATH)

    return vcs_rev, dev_num, version


def _get_version(path):
    try:
        from karabo.packaging.versioning import git_version
    except ImportError:
        git_version = None

    vcs_rev, dev_num, version = _get_src_dist_version()

    if vcs_rev != 'Unknown':
        # This path is taken when building from a source distribution (sdist)
        return {'vcs_revision': vcs_rev, 'revision_count': dev_num,
                'version': version, 'released': True}
    elif git_version is None:
        # This path is taken when building an arbitrary commit with pip
        return {'vcs_revision': vcs_rev, 'revision_count': dev_num,
                'version': version, 'released': False}

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


class install_with_jupyter(install):
    def run(self):
        super().run()
        install_kernel_spec(
            op.join(op.dirname(__file__), "karabo",
                    "interactive", "jupyter_spec"),
            kernel_name="Karabo", prefix=sys.prefix)


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
        cmdclass={"install": install_with_jupyter},
        package_data={
            'karabo.bound_api.tests': ['resources/*.*'],
            "karabo.common.scenemodel.tests": ["data/*.svg",
                                               "data/inkscape/*.svg",
                                               "data/legacy/*.svg",
                                               "data/legacy/icon_data/*.svg"],
            'karabo.middlelayer_api.tests': ['*.xml'],
            'karabo.testing': ['resources/*.*'],
            'karabo.project_db': ['config_stubs/*.*'],
            'karabo.integration_tests': ['device_comm_test/PPSenderDevice.egg-info/*.*'],
            'karabo.interactive': ['jupyter_spec/kernel.json'],
        },
        entry_points={
            'console_scripts': [
                'karabo=karabo.interactive.karabo:main',
                'karabo-pythonserver=karabo.bound_api.device_server:main',
                'karabo-middlelayerserver=karabo.middlelayer_api.device_server:DeviceServer.main',
                'karabo-cli=karabo.interactive.ideviceclient:main',
                'ikarabo=karabo.interactive.ikarabo:main',
                'convert-karabo-device=karabo.interactive.convert_device_project:main',
                'karabo-scene2py=karabo.interactive.scene2python:main',
                'karabo-start=karabo.interactive.startkarabo:startkarabo',
                'karabo-stop=karabo.interactive.startkarabo:stopkarabo',
                'karabo-kill=karabo.interactive.startkarabo:killkarabo',
                'karabo-check=karabo.interactive.startkarabo:checkkarabo',
                'karabo-gterm=karabo.interactive.startkarabo:gnometermlog',
                'karabo-xterm=karabo.interactive.startkarabo:xtermlog',
                'karabo-add-deviceserver=karabo.interactive.startkarabo:adddeviceserver',
                'karabo-remove-deviceserver=karabo.interactive.startkarabo:removedeviceserver',
                'karabo-webserver=karabo.interactive.webserver:run_webserver',
             ],
            'karabo.bound_device': [
                'ProjectManager=karabo.bound_devices.project_manager:ProjectManager',
                'PropertyTest=karabo.bound_devices.property_test:PropertyTest',
                'RunConfigurationGroup=karabo.bound_devices.run_configuration_group:RunConfigurationGroup',
                'RunConfigurator=karabo.bound_devices.run_configurator:RunConfigurator',
            ],
            'karabo.middlelayer_device': [
                'IPythonKernel=karabo.middlelayer_api.ipython:IPythonKernel',
                'MetaMacro=karabo.middlelayer_api.metamacro:MetaMacro'
            ],
            'karabo.bound_device_test': [
                'TestDevice=karabo.middlelayer_api.tests.bounddevice:TestDevice'
            ]
        },
    )
