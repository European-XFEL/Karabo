#!/usr/bin/env python
from setuptools import setup, find_packages


setup(name='__CLASS_NAME__',
      version="1.0.0",
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
