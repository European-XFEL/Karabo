from setuptools import setup, find_packages

setup(
    name="karabo",
    version="1.3",
    author="Karabo Team",
    author_email="karabo@xfel.eu",
    description=
        "This is the graphical user interface of the Karabo control system",
    url="http://karabo.eu",
    packages=find_packages(),
    package_data={
        'karabo._api1.tests': ['resources/*.*'],
        'karabo.tests.tests_api_2': ['*.xml'],
    },
    entry_points={'console_scripts': [
                  'karabo_device_server=karabo.device_server:main',
                  'ideviceclient=karabo.ideviceclient:main',
                  'ikarabo=karabo.ikarabo:main',
                  'convert-karabo-device-project=karabo.convert_device_project:main',
                  ]},
)
