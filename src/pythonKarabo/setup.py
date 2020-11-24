import os
import os.path as op
import sys

from jupyter_client.kernelspec import install_kernel_spec
from setuptools import setup, find_packages
from setuptools.command.install import install


class InstallWithJupyter(install):
    def run(self):
        super().run()
        install_kernel_spec(
            op.join(op.dirname(__file__), "karabo",
                    "interactive", "jupyter_spec"),
            kernel_name="Karabo", prefix=sys.prefix)


if os.environ.get('BUILD_KARABO_GUI', '0') == '1':
    # We're building the GUI, so we don't need to package everything
    packages = find_packages(include=[
        'karabo', 'karabo.common*', 'karabo.native*', 'karabo.testing*'
    ])

    package_data = {
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.testing': ['resources/*.*'],
    }

    entry_points = {}
else:
    # When building karabo, everything gets included
    packages = find_packages()

    package_data = {
        'karabo.bound_api.tests': ['resources/*.*'],
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.middlelayer_api.tests': ['*.xml'],
        'karabo.project_db': ['config_stubs/*.*'],
        'karabo.integration_tests': [
            'device_comm_test/CommTestDevice.egg-info/*.*',
            'device_provided_scenes_test/SceneProvidingDevice.egg-info/*.*',
            'pipeline_processing_test/PPReceiverDevice.egg-info/*.*',
            'pipeline_processing_test/PPSenderDevice.egg-info/*.*'],
        'karabo.interactive': [
            'jupyter_spec/kernel.json',
            'static/*.css',
            'static/*.js',
            'static/*.html',
            'static/favicon.ico',
            'templates/*.html'],
        'karabo.testing': ['resources/*.*'],
        'karabo.influxdb.tests': ['sample_data/PropertyTestDevice/raw/*.txt'],
    }

    entry_points = {
        'console_scripts': [
            'karabo=karabo.interactive.karabo:main',
            'karabo-pythonserver=karabo.bound_api.device_server:main',
            'karabo-middlelayerserver=karabo.middlelayer_api.device_server:DeviceServer.main',
            'karabo-macroserver=karabo.macro_devices.macro_server:MacroServer.main',
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
            'karabo-webaggregatorserver=karabo.interactive.webaggregatorserver:run_webserver',
            'migrate-karabo-history=karabo.influxdb.dl_migrator:main',
        ],
        'karabo.bound_device': [
            'PropertyTest=karabo.bound_devices.property_test:PropertyTest',
        ],
        'karabo.middlelayer_device': [
            'PropertyTestMDL=karabo.middlelayer_devices.property_test:PropertyTestMDL',
            'ProjectManager=karabo.middlelayer_devices.project_manager:ProjectManager',
            'ConfigurationManager=karabo.middlelayer_devices.configuration_manager:ConfigurationManager',
            'DaemonManager=karabo.middlelayer_devices.daemon_manager:DaemonManager',
        ],
        'karabo.macro_device': [
            'IPythonKernel=karabo.macro_devices.ipython:IPythonKernel',
            'MetaMacro=karabo.macro_devices.metamacro:MetaMacro'
        ],
        'karabo.bound_device_test': [
            'TestDevice=karabo.bound_api.tests.boundDevice:TestDevice'
        ],
    }

CURRENT_FOLDER = os.path.dirname(os.path.realpath(__file__))
VERSION_FILE_PATH = os.path.join(CURRENT_FOLDER, 'karabo', '_version.py')
ROOT_FOLDER = os.path.dirname(os.path.dirname(CURRENT_FOLDER))

if __name__ == '__main__':
    setup(
        name='karabo',
        use_scm_version={'root': ROOT_FOLDER, 'write_to': VERSION_FILE_PATH},
        author='Karabo Team',
        author_email='karabo@xfel.eu',
        description='This is the Python interface of the Karabo control system',
        url='http://karabo.eu',
        packages=packages,
        cmdclass={'install': InstallWithJupyter},
        package_data=package_data,
        entry_points=entry_points
    )
