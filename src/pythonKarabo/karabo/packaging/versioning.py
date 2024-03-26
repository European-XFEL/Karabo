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
import os.path as op
import re
import subprocess
from functools import partial
from os import environ

from packaging.version import InvalidVersion, Version

# NOTE: Only stdlib imports are allowed here! This module is used when
# installing the karabo package.


class SvnParseError(ValueError):
    pass


def _get_development_version(path):
    """ Return the current development version.
    """
    commit_count = '0'
    generators = [partial(f, path)
                  for f in (svn_version, jsvn_version, git_version)]
    for gen in generators:
        vers_dict = gen()
        if vers_dict['vcs_revision'] != 'Unknown':
            commit_count = vers_dict['revision_count']
            break

    return commit_count


def _get_karabo_framework_path():
    """ Get the filesystem location of the running Karabo framework.
    """
    karabo = environ.get('KARABO')
    if karabo is None:
        msg = ("$KARABO env variable not defined! Source the activate script "
               "for the desired Karabo framework to address this issue.")
        raise RuntimeError(msg)

    return karabo


def _parse_svn_part(info_str, part_name):
    """ Extract various information from the output of `svn info`

    part_name = 'revision': Gives the current SVN revision number
    part_name = 'branch': Gives the SVN branch name
    """
    svn_parts = {
        'revision': r'Revision\: (\d+)',
        'branch': r'URL\: .+/(.+)',
    }
    match = re.search(svn_parts[part_name], info_str)
    if match is not None:
        return match.groups()[0]
    raise SvnParseError(f'SVN {part_name} not found!')


def get_karabo_framework_version(strict_fallback=True) -> Version:
    """ Returns the current Karabo version contained in the Framework VERSION
    file.

    Parameter:
    strict_fallback(bool): when True, considers only the "major.minor.patch"
    portion as a fallback strategy in case an invalid version string is read
    from the VERSION file.

    Returns:
    A packaging.version.Version with the value stored in the VERSION file if
    that value (or its "major.minor.patch" portion) is a valid version string.

    Raises:
    A packaging.version.InvalidVersion if the value in the VERSION file is
    an invalid version string even after the fallback strategy is applied.
    """
    str_version = _read_version_file()
    version = None
    try:
        version = Version(str_version)
    except InvalidVersion:
        if strict_fallback:
            str_version = _filter_major_minor_patch(str_version)
            version = Version(str_version)
        else:
            raise
    return version


def _read_version_file() -> str:
    karabo_path = _get_karabo_framework_path()
    version_path = op.join(karabo_path, 'VERSION')
    with open(version_path) as fp:
        version = fp.read().strip()
    return version


def _filter_major_minor_patch(version_str: str) -> str:
    pattern = r'(?P<major>\d+)\.(?P<minor>\d+)\.?(?P<patch>\d*)?'
    matches = re.match(pattern, version_str)
    return matches[0] if matches else ""


def get_karabo_version():
    """Return the current Karabo Python package version."""
    try:
        from karabo._version import full_version
    except ImportError:
        from karabo._version import version
        from karabo.common.packaging import utils
        full_version = utils.extract_full_version(version)
    return full_version


def get_package_version(path):
    """ Get the package version for a device's repository.

    Returns a PEP 440 compatible version string which is a combination of the
    active Karabo framework's version and the commit count of the device's
    repository.
    """
    path = op.normpath(path)
    karabo_version = get_karabo_version()
    dev_version = _get_development_version(path)
    extra = f'.dev{dev_version}' if dev_version != '0' else ''

    return karabo_version + extra


def git_version(path):
    """ Return the git revision as a string and a commit count
    """
    try:
        # Find a tag matching the glob *.*.* (version number prefixed tags)
        cmd = 'git describe --match *.*.*'.split(' ')
        out = subprocess.check_output(cmd, cwd=path).decode('ascii')
    except (subprocess.CalledProcessError, OSError):
        out = ''

    # Git will return something like NN.NN.NN[(a|b|rc)NN]?-NN-XXXXXX
    # The group names in this regex explain the parts
    # The only required part of the regex is the NN.NN.NN. All others optional.
    expr = (
        r'(?P<major>\d+)\.(?P<minor>\d+)\.(?P<micro>\d+)(?P<abrc>(a|b|rc)\d+)?'
        r'(\-)?(?P<count>\d+)?(-g)?(?P<hash>[a-fA-F0-9]+)?'
    )
    match = re.match(expr, out)
    if match is None:
        revision, count = 'Unknown', '0'
        version, released = '0.0.0', False
    else:
        revision = match.group('hash') or ''
        count = match.group('count') or '0'
        version = '.'.join([match.group(n)
                            for n in ('major', 'minor', 'micro')])
        released = (revision == '' and count == '0')

    return {
        'vcs_revision': revision,
        'revision_count': count,
        'version': version,
        'released': released,
    }


def svn_version(path, svn_cmd='svn'):
    """ Return the SVN revision as a string and a commit count.
    """
    cmd = [svn_cmd, 'info', path]
    try:
        out = subprocess.check_output(cmd).decode('ascii')
        svn_branch = _parse_svn_part(out, 'branch')
        svn_count = _parse_svn_part(out, 'revision')
    except (subprocess.CalledProcessError, OSError, SvnParseError):
        return {'vcs_revision': 'Unknown', 'revision_count': '0',
                'version': '0.0.0', 'released': False}

    return {
        'vcs_revision': f'{svn_branch}@r{svn_count}',
        'revision_count': svn_count,
        'version': svn_branch,
        'released': svn_count == '0',
    }


# Define this where it can be imported.
jsvn_version = partial(svn_version, svn_cmd='jsvn')


class device_scm_version:
    """Backward compatible karabo device tags compliant to PEP440"""

    def __init__(self, root_path, file_path):
        self.root = root_path
        self.write_to = file_path

    def __call__(self):
        """setuptools_scm allows a callable to be specified
        in the setup function"""
        return {
            "root": self.root,
            "write_to": self.write_to,
        }
