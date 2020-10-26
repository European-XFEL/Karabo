#! /usr/bin/env python

from setuptools import setup, find_packages

requirements = []

setup(name='cppunitxmlparser',
      version='0.0.2',
      license='MIT',
      author='Alessandro Silenzi',
      author_email='alessandro.silenzi@xfel.eu',
      description='A tool to merge cppunit provided xmls',
      long_description='',
      packages=find_packages(),
      package_data={},
      entry_points={
          'console_scripts': [
              'cppunitxml-merge = cppunitxmlparser.merge_cppunitxml:main',
              'cppunitxml-check = cppunitxmlparser.merge_check:main',
              'cppunitxml-junitmerge = cppunitxmlparser.merge_junit:main',
          ],
      },
      requires=requirements,
      )
