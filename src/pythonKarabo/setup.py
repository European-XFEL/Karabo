# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# flake8: noqa
import os

from setuptools import find_packages, setup

CURRENT_FOLDER = os.path.dirname(os.path.realpath(__file__))
VERSION_FILE_PATH = os.path.join(CURRENT_FOLDER, 'karabo', '_version.py')
ROOT_FOLDER = os.path.dirname(os.path.dirname(CURRENT_FOLDER))
# Karabo can be installed in different optional configurations
# until optional dependencies
# https://setuptools.pypa.io/en/latest/
# userguide/dependency_management.html#optional-dependencies
# are not implemented, one can tune this configurations with an
# environment variable called BUILD_KARABO_SUBMODULE.
# this can be set to:
# - NATIVE
#   to install only the common module and
#   the native hash and schema binding module
# - MDL
#   to install the modules in NATIVE plus the
#   asyncio devices library middlelayer
# - anything else:
#   to install all modules.
SUBMODULE = os.environ.get('BUILD_KARABO_SUBMODULE', '')


install_args = {
    'name': 'karabo',
    'use_scm_version': {'root': ROOT_FOLDER, 'write_to': VERSION_FILE_PATH},
    'author': 'Karabo Team',
    'author_email': 'opensource@xfel.eu',
    'description': 'This is the Python interface of the Karabo control system',
    'url': 'http://karabo.eu',
    'license': "MPL2",
}

if SUBMODULE == 'NATIVE':
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

elif SUBMODULE == 'MDL':
    install_args['packages'] = find_packages(include=[
        'karabo', 'karabo.common*', 'karabo.native*', 'karabo.testing*',
        'karabo.interactive*', 'karabo.middlelayer*',
        'karabo.middlelayer_devices*',
        'karabo.packaging*',
    ])

    install_args['package_data'] = {
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.middlelayer.tests': ['*.xml'],
        'karabo.testing': ['resources/*.*'],
    }

    install_args['entry_points'] = {
        'console_scripts': [
            'karabo=karabo.interactive.karabo:main',
            'karabo-middlelayerserver=karabo.middlelayer.device_server:MiddleLayerDeviceServer.main',
            'ikarabo=karabo.interactive.ikarabo:main',
        ],
        'karabo.middlelayer_device': [
            'PropertyTest=karabo.middlelayer_devices.property_test:PropertyTest',
        ],
    }
else:
    # When building karabo, everything gets included
    install_args['packages'] = find_packages()

    install_args['package_data'] = {
        'karabo.bound.tests': ['resources/*.*'],
        'karabo.common.scenemodel.tests': [
            'data/*.svg', 'data/inkscape/*.svg', 'data/legacy/*.svg',
            'data/legacy/icon_data/*.svg'
        ],
        'karabo.middlelayer.tests': ['*.xml'],
        'karabo.project_db': ['config_stubs/*.*'],
        'karabo.interactive': [
            'static/*.css',
            'static/*.js',
            'static/*.html',
            'static/favicon.ico',
            'templates/*.html',
            'tests/karaboDB'],
        'karabo.testing': ['resources/*.*'],
        'karabo.influx_db.tests': ['sample_data/PropertyTestDevice/raw/*.txt'],
    }

    install_args['entry_points'] = {
        'console_scripts': [
            'karabo=karabo.interactive.karabo:main',
            'karabo-pythonserver=karabo.bound.device_server:main',
            'karabo-middlelayerserver=karabo.middlelayer.device_server:DeviceServer.main',
            'karabo-macroserver=karabo.middlelayer.macro_server:MacroServer.main',
            'karabo-cli=karabo.interactive.ideviceclient:main',
            'ikarabo=karabo.interactive.ikarabo:main',
            'convert-karabo-device=karabo.interactive.convert_device_project:main',
            'migrate-configdb=karabo.interactive.convert_config_db:main',
            'karabo-scene2cpp=karabo.interactive.scene2cpp:main',
            'karabo-scene2py=karabo.interactive.scene2python:main',
            'karabo-start=karabo.interactive.startkarabo:startkarabo',
            'karabo-stop=karabo.interactive.startkarabo:stopkarabo',
            'karabo-kill=karabo.interactive.startkarabo:killkarabo',
            'karabo-check=karabo.interactive.startkarabo:checkkarabo',
            'karabo-gterm=karabo.interactive.startkarabo:gnometermlog',
            'karabo-xterm=karabo.interactive.startkarabo:xtermlog',
            'karabo-less=karabo.interactive.startkarabo:less',
            'karabo-add-deviceserver=karabo.interactive.startkarabo:adddeviceserver',
            'karabo-remove-deviceserver=karabo.interactive.startkarabo:removedeviceserver',
            'karabo-create-services=karabo.interactive.startkarabo:make_service_dir',
            'karabo-webserver=karabo.interactive.webserver:run_webserver',
            'karabo-webaggregatorserver=karabo.interactive.webaggregatorserver:run_webserver',
            'migrate-karabo-history=karabo.influx_db.dl_migrator:main',
            'karabo-check-container=karabo.interactive.container_monitor:main',
        ],
        'karabo.bound_device': [
            'PropertyTest=karabo.bound_devices.property_test:PropertyTest',
        ],
        'karabo.middlelayer_device': [
            'PropertyTest=karabo.middlelayer_devices.property_test:PropertyTest',
            'ProjectManager=karabo.middlelayer_devices.project_manager:ProjectManager',
            'ConfigurationManager=karabo.middlelayer_devices.configuration_manager:ConfigurationManager',
            'DaemonManager=karabo.middlelayer_devices.daemon_manager:DaemonManager',
        ],
        'karabo.middlelayer_device_test': [
            'MiddlelayerDevice=karabo.integration_tests.device_cross_test.test_cross:MiddlelayerDevice',
            'MdlOrderTestDevice=karabo.integration_tests.signal_slot_order_test.mdl_ordertest_device:MdlOrderTestDevice',
        ],
        'karabo.macro_device': [
            'MetaMacro=karabo.middlelayer.metamacro:MetaMacro'
        ],
        'karabo.bound_device_test': [
            'TestDevice=karabo.bound.tests.boundDevice:TestDevice',
            'CommTestDevice=karabo.integration_tests.device_comm_test.commtestdevice:CommTestDevice',
            'SceneProvidingDevice=karabo.integration_tests.device_provided_scenes_test.scene_providing_device:SceneProvidingDevice',
            'NonSceneProvidingDevice=karabo.integration_tests.device_provided_scenes_test.non_scene_providing_device:NonSceneProvidingDevice',
            'DeviceWithLimit=karabo.integration_tests.device_schema_injection_test.device_with_limit:DeviceWithLimit',
            'DeviceWithTableElementParam=karabo.integration_tests.device_schema_injection_test.device_with_table_parameter:DeviceWithTableElementParam',
            'DeviceChannelInjection=karabo.integration_tests.device_schema_injection_test.device_channel_injection:DeviceChannelInjection',
            'PPSenderDevice=karabo.integration_tests.pipeline_processing_test.ppsender:PPSenderDevice',
            'PPReceiverDevice=karabo.integration_tests.pipeline_processing_test.ppreceiver:PPReceiverDevice',
            'UnstoppedThreadDevice=karabo.integration_tests.device_comm_test.unstoppedThreadDevice:UnstoppedThreadDevice',
            'SlowStartDevice=karabo.integration_tests.device_comm_test.slowStartDevice:SlowStartDevice',
            'StuckLoggerDevice=karabo.integration_tests.device_comm_test.stuckLoggerDevice:StuckLoggerDevice',
            'RaiseInitializationDevice=karabo.integration_tests.device_comm_test.raiseInitializationDevice:RaiseInitializationDevice',
            'RaiseOnDunderInitDevice=karabo.integration_tests.device_comm_test.raiseInitializationDevice:RaiseOnDunderInitDevice',
            'InvalidImportDevice=karabo.integration_tests.device_comm_test.invalidImportDevice:InvalidImportDevice',
            'BoundOrderTestDevice=karabo.integration_tests.signal_slot_order_test.bound_ordertest_device:BoundOrderTestDevice',
        ],
        'karabo.bound_broken_device_test': [
            'BrokenTestDevice=karabo.bound.tests.brokenBoundDevice:BrokenTestDevice',
        ],
    }


if __name__ == '__main__':
    setup(**install_args)
