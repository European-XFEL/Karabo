import argparse
from contextlib import contextmanager
import copy
import os
import os.path as op
import stat
import sys

from karabo.api_1 import PythonDevice

BUILD_PACKAGE_FILE = """\
#!/bin/bash

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
fi

$KARABO/bin/.bundle-pythonplugin.sh $@
"""
ENTRY_POINT_TEMPLATE = "'{cname} = {package_name}.{fname}:{cname}',"
SETUP_TEMPLATE = """\
#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='{package_name}',
      version='',
      author='',
      author_email='',
      description='',
      long_description='',
      url='',
      package_dir={{'': 'src'}},
      packages=find_packages('src'),
      entry_points={{
          'karabo.python_device.api_1': [{entry_points}
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


def create_package_directory(package_name, directory, common_prefix):
    """ Move all the Python source files to a new package directory.
    """
    strip_common_prefix = lambda s: '...' + s[len(common_prefix):]
    package_dir = op.join(directory, package_name)
    if op.exists(package_dir):
        print('Package directory:', package_dir, 'already exists!. Exiting.')
        sys.exit(1)

    print('Creating package directory:', strip_common_prefix(package_dir))
    os.makedirs(package_dir)
    with open(op.join(package_dir, '__init__.py'), 'wt'):
        pass

    print('Moving Files:')
    filenames = [fn for fn in os.listdir(directory) if fn.endswith('.py')]
    for fn in filenames:
        src = op.join(directory, fn)
        dst = op.join(package_dir, fn)
        print(strip_common_prefix(src), '=>', strip_common_prefix(dst))
        os.rename(src, dst)


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
    with open(path, 'r') as fp:
        try:
            code = compile(fp.read(), op.basename(path), 'exec')
            exec(code, mod_dict)
        except (ImportError, SyntaxError) as ex:
            mod_dict.clear()
            error = '{}: {}'.format(type(ex).__name__, str(ex))

    return error


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
            print('  In "{}" =>'.format(fn), error)
    print('Exiting.')


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

    # Find all the devices defined in the project
    device_files, errors = scan_modules(src_dir)
    # Bail out when no devices are found
    if len(device_files) == 0:
        show_device_errors(errors)
        sys.exit(1)

    setup_py = build_setup_py_file(package_name, device_files)
    # Move *.py files to a new package directory
    common_prefix = op.dirname(directory)
    create_package_directory(package_name, src_dir, common_prefix)

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
