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
