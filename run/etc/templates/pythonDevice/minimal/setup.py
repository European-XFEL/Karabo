#! /usr/bin/env python

from setuptools import setup, find_packages

setup(name='__PACKAGE_NAME__',
      version='',
      author='',
      author_email='__EMAIL__',
      description='',
      long_description='',
      url='',
      package_dir={'': 'src'},
      packages=find_packages('src'),
      entry_points={
          'karabo.python_device.api_1': [
              '__CLASS_NAME__ = __PACKAGE_NAME__.__CLASS_NAME__:__CLASS_NAME__',
          ],
      },
      package_data={},
      requires=[],
      )

