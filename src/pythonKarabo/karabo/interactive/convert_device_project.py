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
import argparse
import copy
import os
import os.path as op
import stat
import subprocess
import sys
from contextlib import contextmanager

from karabo.bound import PythonDevice

BUILD_PACKAGE_FILE = r"""\
#!/bin/bash

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate\
 script for the Karabo Framework which you would like to use."
  exit 1
fi

$KARABO/bin/.bundle-pythonplugin.sh $@
"""
ENTRY_POINT_TEMPLATE = "'{cname} = {package_name}.{fname}:{cname}',"
SETUP_TEMPLATE = """\
#!/usr/bin/env python

from os.path import dirname

from setuptools import setup, find_packages


def _get_version_string():
    try:
        from karabo.packaging.versioning import get_package_version
    except ImportError:
        print("WARNING: Karabo framework not found! Version will be blank!")
        return ''

    return get_package_version(dirname(__file__))

setup(name='{package_name}',
      version=_get_version_string(),
      author='',
      author_email='',
      description='',
      long_description='',
      url='',
      package_dir={{'': 'src'}},
      packages=find_packages('src'),
      entry_points={{
          'karabo.bound_device': [{entry_points}
          ],
      }},
      package_data={{}},
      requires=[],
      )
"""


def build_setup_py_file(package_name, device_files):
    """ Fill in the setup.py template with the devices that were found.
    """
    entry_points = ['']
    kwargs = {'package_name': package_name}
    for (path, cname) in device_files:
        kwargs['fname'] = op.splitext(path)[0]
        kwargs['cname'] = cname
        entry_points.append(ENTRY_POINT_TEMPLATE.format(**kwargs))

    kwargs = {
        'package_name': package_name,
        'entry_points': ('\n' + ' '*14).join(entry_points),
    }

    return SETUP_TEMPLATE.format(**kwargs)


def strip_common_prefix(s, common_prefix):
    return '...' + s[len(common_prefix):]


def create_package_directory(package_name, directory, common_prefix,
                             use_svn=False):
    """ Move all the Python source files to a new package directory.
    """
    rename = svn_rename if use_svn else os.rename
    package_dir = op.join(directory, package_name)
    if op.exists(package_dir):
        print('Package directory:', package_dir, 'already exists!. Exiting.')
        sys.exit(1)

    print('Creating package directory:',
          strip_common_prefix(package_dir, common_prefix))
    os.makedirs(package_dir)
    with open(op.join(package_dir, '__init__.py'), 'w'):
        pass

    if use_svn:
        args = ['svn', 'add', package_name]
        subprocess.check_call(args, cwd=directory)

    print('Moving Files:')
    filenames = [fn for fn in os.listdir(directory) if fn.endswith('.py')]
    for fn in filenames:
        src = op.join(directory, fn)
        dst = op.join(package_dir, fn)
        print(strip_common_prefix(src, common_prefix), '=>',
              strip_common_prefix(dst, common_prefix))
        rename(src, dst)


@contextmanager
def dir_in_sys_path(directory):
    """ Temporarily add a directory to the system path.
    """
    old_path = copy.copy(sys.path)
    sys.path.append(directory)
    yield
    sys.path = old_path


def execfile(path, mod_dict):
    """ This function is not present in Python 3. This is a simple
    implementation which gets the job done.
    """
    error = None
    with open(path) as fp:
        try:
            code = compile(fp.read(), op.basename(path), 'exec')
            exec(code, mod_dict)
        except (ImportError, SyntaxError) as ex:
            mod_dict.clear()
            error = f'{type(ex).__name__}: {str(ex)}'

    return error


def is_svn(directory):
    """ Determine if a directory is tracked by SVN.
    """
    return op.exists(op.join(directory, '.svn'))


def scan_modules(directory):
    """ For every python source file in a directory, check to see if it
    contains the definition of a device.
    """
    found_devices = []
    errors = {}

    filenames = [fn for fn in os.listdir(directory) if fn.endswith('.py')]
    with dir_in_sys_path(directory):
        for fn in filenames:
            mod_dict = {}
            error = execfile(op.join(directory, fn), mod_dict)
            for name, obj in mod_dict.items():
                try:
                    is_pd_subclass = issubclass(obj, PythonDevice)
                except TypeError:
                    is_pd_subclass = False
                if is_pd_subclass and obj is not PythonDevice:
                    found_devices.append((fn, name))

            if error:
                errors[fn] = error

    return found_devices, errors


def show_device_errors(errors):
    """ Print out a dictionary of errors.
    """
    print('Error: No devices found!')
    if errors:
        print('The following errors were encountered:')
        for fn, error in errors.items():
            print(f'  In "{fn}" =>', error)
    print('Exiting.')


def svn_rename(src, dst):
    cwd = op.dirname(src)
    src = src[len(cwd)+1:]
    dst = dst[len(cwd)+1:]
    args = ['svn', 'mv', src, dst]
    subprocess.check_call(args, cwd=cwd)


def main():
    description = ('Convert an old-style PythonDevice project to one which '
                   'builds as a standard Python package. NOTE: This will '
                   'make changes to you project! Make sure you have a way '
                   'to revert the changes if something goes wrong!!!')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('directory',
                        help='The directory of the project to convert.')

    ns = parser.parse_args()

    directory = op.normpath(op.abspath(ns.directory))
    package_name = op.basename(directory)
    src_dir = op.join(directory, 'src')
    builder_path = op.join(directory, 'build-package.sh')
    setup_path = op.join(directory, 'setup.py')
    if op.exists(setup_path):
        print('"setup.py" already exists! Exiting.')
        sys.exit(1)

    # Use SVN?
    use_svn = is_svn(directory)

    # Find all the devices defined in the project
    device_files, errors = scan_modules(src_dir)
    # Bail out when no devices are found
    if len(device_files) == 0:
        show_device_errors(errors)
        sys.exit(1)

    setup_py = build_setup_py_file(package_name, device_files)
    # Move *.py files to a new package directory
    common_prefix = op.dirname(directory)
    create_package_directory(package_name, src_dir, common_prefix,
                             use_svn=use_svn)

    # Write setup.py
    with open(setup_path, 'w') as fp:
        print(setup_py, file=fp)

    # Move the old build-package.sh script
    if op.exists(builder_path):
        old_builder = op.join(directory, 'build-package-old.sh')
        os.rename(builder_path, old_builder)

    # Write the newer build-package.sh script
    with open(builder_path, 'w') as fp:
        print(BUILD_PACKAGE_FILE, file=fp)

    # Make sure the builder script is executable.
    builder_mode = os.stat(builder_path).st_mode
    builder_mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    os.chmod(builder_path, builder_mode)


if __name__ == '__main__':
    main()
