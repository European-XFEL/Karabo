#!/usr/bin/env python
# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
from os.path import dirname, join, realpath

from setuptools import find_packages, setup

from karabo.packaging.versioning import device_scm_version

ROOT_FOLDER = dirname(realpath(__file__))
scm_version = device_scm_version(
    ROOT_FOLDER,
    join(ROOT_FOLDER, 'src', '__PACKAGE_NAME__', '_version.py')
)


setup(name='__CLASS_NAME__',
      use_scm_version=scm_version,
      author='__EMAIL__',
      author_email='__EMAIL__',
      description='',
      long_description='',
      url='',
      package_dir={'': 'src'},
      packages=find_packages('src'),
      entry_points={
          'karabo.middlelayer_device': [
              '__CLASS_NAME__ = __PACKAGE_NAME__.__CLASS_NAME__:__CLASS_NAME__'
          ],
      },
      package_data={},
      requires=[],
      )
