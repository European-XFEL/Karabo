#! /usr/bin/env python
from os.path import dirname

from setuptools import setup, find_packages


def _get_version_string():
    try:
        from karabo.packaging.versioning import get_package_version
    except ImportError:
        print("WARNING: Karabo framework not found! Version will be blank!")
        return ''

    return get_package_version(dirname(__file__))

setup(name='__PACKAGE_NAME__',
      version=_get_version_string(),
      author='',
      author_email='__EMAIL__',
      description='',
      long_description='',
      url='',
      package_dir={'': 'src'},
      packages=find_packages('src'),
      entry_points={
          'karabo.bound_device': [
              '__CLASS_NAME__ = __PACKAGE_NAME__.__CLASS_NAME__:__CLASS_NAME__',
          ],
          'console_scripts': [
              '__CLASS_NAME__-server = __PACKAGE_NAME__.server.__CLASS_NAME__:main',
          ],
      },
      package_data={},
      requires=[],
      )
