import os
import os.path as op
import sys

from setuptools import find_packages, setup

CURRENT_FOLDER = os.path.dirname(os.path.realpath(__file__))
VERSION_FILE_PATH = os.path.join(CURRENT_FOLDER, 'karabo', '_version.py')
ROOT_FOLDER = os.path.dirname(os.path.dirname(CURRENT_FOLDER))

install_args = {
    'name': 'karabo',
    'use_scm_version': {'root': ROOT_FOLDER, 'write_to': VERSION_FILE_PATH},
    'author': 'Karabo Team',
    'author_email': 'karabo@xfel.eu',
    'description': 'This is the Python interface of the Karabo control system',
    'url': 'http://karabo.eu',
}

if os.environ.get('BUILD_KARABO_SUBMODULE', '') == 'NATIVE':
    # We're building the GUI, so we don't need to package everything
    install_args['packages'] = find_packages(include=[
        'karabo', 'karabo.common*', 'karabo.native*', 'karabo.testing*'
    ])

    install_args['package_data'] = {
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.testing': ['resources/*.*'],
    }

elif os.environ.get('BUILD_KARABO_SUBMODULE', '') == 'MDL':
    install_args['packages'] = find_packages(include=[
        'karabo', 'karabo.common*', 'karabo.native*', 'karabo.testing*',
        'karabo.interactive*', 'karabo.middlelayer_api*',
        'karabo.middlelayer_devices*',
        'karabo.packaging*',
    ])

    install_args['package_data'] = {
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.middlelayer_api.tests': ['*.xml'],
        'karabo.testing': ['resources/*.*'],
    }

    install_args['entry_points'] = {
        'console_scripts': [
            'karabo-middlelayerserver=karabo.middlelayer_api.device_server:DeviceServer.main',
            'ikarabo=karabo.interactive.ikarabo:main',
        ],
        'karabo.middlelayer_device': [
            'PropertyTestMDL=karabo.middlelayer_devices.property_test:PropertyTestMDL',
        ],
    }
else:
    # When building karabo, everything gets included
    install_args['packages'] = find_packages()

    install_args['package_data'] = {
        'karabo.bound_api.tests': ['resources/*.*'],
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.middlelayer_api.tests': ['*.xml'],
        'karabo.project_db': ['config_stubs/*.*'],
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

    install_args['entry_points'] = {
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
            'karabo-create-services=karabo.interactive.startkarabo:make_service_dir',
            'karabo-webserver=karabo.interactive.webserver:run_webserver',
            'karabo-webaggregatorserver=karabo.interactive.webaggregatorserver:run_webserver',
            'migrate-karabo-history=karabo.influxdb.dl_migrator:main',
            'karabo-check-container=karabo.interactive.container_monitor:main',
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
        'karabo.middlelayer_device_test': [
            'MiddlelayerDevice=karabo.integration_tests.device_cross_test.test_cross:MiddlelayerDevice',
            'MdlOrderTestDevice=karabo.integration_tests.signal_slot_order_test.mdl_ordertest_device:MdlOrderTestDevice',
        ],
        'karabo.macro_device': [
            'IPythonKernel=karabo.macro_devices.ipython:IPythonKernel',
            'MetaMacro=karabo.macro_devices.metamacro:MetaMacro'
        ],
        'karabo.bound_device_test': [
            'TestDevice=karabo.bound_api.tests.boundDevice:TestDevice',
            'CommTestDevice=karabo.integration_tests.device_comm_test.commtestdevice:CommTestDevice',
            'SceneProvidingDevice=karabo.integration_tests.device_provided_scenes_test.scene_providing_device:SceneProvidingDevice',
            'NonSceneProvidingDevice=karabo.integration_tests.device_provided_scenes_test.non_scene_providing_device:NonSceneProvidingDevice',
            'DeviceWithAlarm=karabo.integration_tests.device_schema_injection_test.device_with_alarm:DeviceWithAlarm',
            'DeviceWithTableElementParam=karabo.integration_tests.device_schema_injection_test.device_with_table_parameter:DeviceWithTableElementParam',
            'DeviceChannelInjection=karabo.integration_tests.device_schema_injection_test.device_channel_injection:DeviceChannelInjection',
            'PPSenderDevice=karabo.integration_tests.pipeline_processing_test.ppsender:PPSenderDevice',
            'PPReceiverDevice=karabo.integration_tests.pipeline_processing_test.ppreceiver:PPReceiverDevice',
            'UnstoppedThreadDevice=karabo.integration_tests.device_comm_test.unstoppedThreadDevice:UnstoppedThreadDevice',
            'SlowStartDevice=karabo.integration_tests.device_comm_test.slowStartDevice:SlowStartDevice',
            'StuckLoggerDevice=karabo.integration_tests.device_comm_test.stuckLoggerDevice:StuckLoggerDevice',
            'RaiseInitializationDevice=karabo.integration_tests.device_comm_test.raiseInitializationDevice:RaiseInitializationDevice',
            'InvalidImportDevice=karabo.integration_tests.device_comm_test.invalidImportDevice:InvalidImportDevice',
            'BoundOrderTestDevice=karabo.integration_tests.signal_slot_order_test.bound_ordertest_device:BoundOrderTestDevice',
        ],
        'karabo.bound_broken_device_test': [
            'BrokenTestDevice=karabo.bound_api.tests.brokenBoundDevice:BrokenTestDevice',
        ],
        'karabo.project_db': [
            'file_db=karabo.project_db.file_db.node:DbConnectionNode',
        ],
    }
    try:
        import eulexistdb
        install_args['entry_points']['karabo.project_db'].append(
            'exist_db=karabo.project_db.exist_db.node:DbConnectionNode'
        )
    except ImportError:
        pass

    try:
        from jupyter_client.kernelspec import install_kernel_spec
        from setuptools.command.develop import develop
        from setuptools.command.install import install

        class WithJupyter():
            def run(self):
                super().run()
                install_kernel_spec(
                    op.join(op.dirname(__file__), "karabo",
                            "interactive", "jupyter_spec"),
                    kernel_name="Karabo", prefix=sys.prefix)

        class InstallWithJupyter(WithJupyter, install):
            pass

        class DevelopWithJupyter(WithJupyter, develop):
            pass

        install_args['cmdclass'] = {
            'install': InstallWithJupyter,
            'develop': DevelopWithJupyter
        }
    except ImportError:
        pass


if __name__ == '__main__':
    setup(**install_args)
