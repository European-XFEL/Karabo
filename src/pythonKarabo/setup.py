from setuptools import setup, find_packages

setup(
    name="karabo",
    version="1.3",
    packages=find_packages(exclude=["tests", "tests.*"]),
    author="Karabo Team",
    author_email="karabo@xfel.eu",
    description=
        "This is the graphical user interface of the Karabo control system",
    url="http://karabo.eu",
    entry_points={'console_scripts': [
                  'karabo_device_server=karabo.device_server:main',
                  'ideviceclient=karabo.ideviceclient:main',
                  'ikarabo=karabo.ikarabo:main',
                  ]},
)
