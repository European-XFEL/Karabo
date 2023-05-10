#!/bin/env python3
#
# The first part of this program is from grin <https://github.com/rkern/grin>
# It's license is shown below.
#
#
# This software is OSI Certified Open Source Software.
# OSI Certified is a certification mark of the Open Source Initiative.
#
# Copyright (c) 2007, Enthought, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#  * Neither the name of Enthought, Inc. nor the names of its contributors may
#    be used to endorse or promote products derived from this software without
#    specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function

import argparse as ap
import os
import os.path as op
import re
import stat
import struct
import tokenize

# Use file(1)'s choices for what's text and what's not.
TEXTCHARS = [7, 8, 9, 10, 12, 13, 27] + list(range(0x20, 0x100))
TEXTCHARS = struct.pack('B'*len(TEXTCHARS), *TEXTCHARS)
ALLBYTES = struct.pack('B'*256, *range(256))


class FileRecognizer(object):
    """ Configurable way to determine what kind of file something is.

    Attributes
    ----------
    skip_hidden_dirs : bool
        Whether to skip recursing into hidden directories, i.e. those starting
        with a "." character.
    skip_hidden_files : bool
        Whether to skip hidden files.
    skip_backup_files : bool
        Whether to skip backup files.
    skip_dirs : container of str
        A list of directory names to skip. For example, one might want to skip
        directories named "CVS".
    skip_exts : container of str
        A list of file extensions to skip. For example, some file names like
        ".so" are known to be binary and one may want to always skip them.
    skip_symlink_dirs : bool
        Whether to skip symlinked directories.
    skip_symlink_files : bool
        Whether to skip symlinked files.
    binary_bytes : int
        The number of bytes to check at the beginning and end of a file for
        binary characters.
    """

    def __init__(self, skip_hidden_dirs=False, skip_hidden_files=False,
                 skip_backup_files=False, skip_dirs=set(), skip_exts=set(),
                 skip_symlink_dirs=True, skip_symlink_files=True,
                 binary_bytes=4096):
        self.skip_hidden_dirs = skip_hidden_dirs
        self.skip_hidden_files = skip_hidden_files
        self.skip_backup_files = skip_backup_files
        self.skip_dirs = skip_dirs

        # For speed, split extensions into the simple ones, that are
        # compatible with os.path.splitext and hence can all be
        # checked for in a single set-lookup, and the weirdos that
        # can't and therefore must be checked for one at a time.
        self.skip_exts_simple = set()
        self.skip_exts_endswith = list()
        for ext in skip_exts:
            if op.splitext('foo.bar'+ext)[1] == ext:
                self.skip_exts_simple.add(ext)
            else:
                self.skip_exts_endswith.append(ext)

        self.skip_symlink_dirs = skip_symlink_dirs
        self.skip_symlink_files = skip_symlink_files
        self.binary_bytes = binary_bytes

    def is_binary(self, filename):
        """ Determine if a given file is binary or not.

        Parameters
        ----------
        filename : str

        Returns
        -------
        is_binary : bool
        """
        with open(filename, 'rb') as f:
            return self._is_binary_file(f)

    def _is_binary_file(self, f):
        """ Determine if a given filelike object has binary data or not.

        Parameters
        ----------
        f : filelike object

        Returns
        -------
        is_binary : bool
        """
        try:
            data = f.read(self.binary_bytes)
        except Exception as e:
            # When trying to read from something that looks like a gzipped
            # file, it may be corrupt. If we do get an error, assume that the
            # file is binary.
            return True
        return bool(data.translate(ALLBYTES, TEXTCHARS))

    def recognize(self, filename):
        """ Determine what kind of thing a filename represents.

        It will also determine what a directory walker should do with the
        file:

            'text' :
                It should should be handled like regular text
            'binary' :
                The file is binary and should be ignored
            'directory' :
                The filename refers to a readable and executable directory that
                should be recursed into if we are configured to do so.
            'link' :
                The filename refers to a symlink that should be skipped.
            'unreadable' :
                The filename cannot be read (and also, in the case of
                directories, is not executable either).
            'skip' :
                The filename, whether a directory or a file, should be skipped
                for any other reason.

        Parameters
        ----------
        filename : str

        Returns
        -------
        kind : str
        """
        try:
            st_mode = os.stat(filename, follow_symlinks=False).st_mode
            if stat.S_ISREG(st_mode):
                return self.recognize_file(filename)
            elif stat.S_ISDIR(st_mode):
                return self.recognize_directory(filename)
            elif stat.S_ISLNK(st_mode):
                return 'link'
            else:
                # We're only interested in regular files and directories.
                # A named pipe in particular would be problematic, because
                # it would cause open() to hang indefinitely.
                return 'skip'
        except OSError:
            return 'unreadable'

    def recognize_directory(self, filename):
        """ Determine what to do with a directory.
        """
        basename = op.split(filename)[-1]
        if (self.skip_hidden_dirs and basename.startswith('.') and
                basename not in ('.', '..')):
            return 'skip'
        if self.skip_symlink_dirs and op.islink(filename):
            return 'link'
        if basename in self.skip_dirs:
            return 'skip'
        return 'directory'

    def recognize_file(self, filename):
        """ Determine what to do with a file.
        """
        basename = op.split(filename)[-1]
        if self.skip_hidden_files and basename.startswith('.'):
            return 'skip'
        if self.skip_backup_files and basename.endswith('~'):
            return 'skip'
        if self.skip_symlink_files and op.islink(filename):
            return 'link'

        filename_nc = op.normcase(filename)
        ext = op.splitext(filename_nc)[1]
        if ext in self.skip_exts_simple or ext.startswith('.~'):
            return 'skip'
        for ext in self.skip_exts_endswith:
            if filename_nc.endswith(ext):
                return 'skip'
        try:
            if self.is_binary(filename):
                return 'binary'
            else:
                return 'text'
        except (OSError, IOError):
            return 'unreadable'

    def walk(self, startpath):
        """ Walk the tree from a given start path yielding all of the files
        (not directories) and their kinds underneath it depth first.

        Paths which are recognized as 'skip', 'link', or 'unreadable' will
        simply be passed over without comment.

        Parameters
        ----------
        startpath : str

        Yields
        ------
        filename : str
        kind : str
        """
        kind = self.recognize(startpath)
        if kind in ('binary', 'text'):
            yield startpath, kind
            # Not a directory, so there is no need to recurse.
            return
        elif kind == 'directory':
            try:
                basenames = os.listdir(startpath)
            except OSError:
                return
            for basename in sorted(basenames):
                path = op.join(startpath, basename)
                for fn, k in self.walk(path):
                    yield fn, k


def get_recognizer():
    skip_dirs = 'CVS,RCS,.svn,.hg,.git,.bzr,build,dist'
    skip_exts = ('.pyc,.pyo,.so,.o,.a,.tgz,.tar.gz,.gz,.rar,.zip,~,#,.bak,.png'
                 ',.jpg,.gif,.bmp,.tif,.tiff,.ppm,.fits,.pyd,.dll,.exe,.obj'
                 ',.lib')
    skip_dirs = set([x for x in skip_dirs.split(',') if x])
    skip_exts = set([x for x in skip_exts.split(',') if x])
    return FileRecognizer(
        skip_hidden_files=True,
        skip_backup_files=True,
        skip_hidden_dirs=True,
        skip_dirs=skip_dirs,
        skip_exts=skip_exts,
        skip_symlink_files=True,
        skip_symlink_dirs=True,
    )

#############################################################################
# Below this line is XFEL code


def fix_paths(build_dir, search_pattern, replacement_text, fix_links=False):
    abs_build_dir = op.abspath(build_dir)
    regex = re.compile(re.escape(search_pattern))
    recognizer = get_recognizer()

    for dirpath, dirname, filenames in os.walk(abs_build_dir):
        for fn in filenames:
            path = op.join(dirpath, fn)
            file_type = recognizer.recognize(path)
            if file_type == 'text':
                substitute_pattern(path, regex, replacement_text)
            elif file_type == 'link' and fix_links:
                substitute_symlink(path, regex, replacement_text)


def substitute_pattern(path, regex, replace_with):
    try:
        with open(path, mode='rb') as fp:
            encoding = tokenize.detect_encoding(fp.readline)[0]
    except SyntaxError:
        print('Unreadable file "{}" ignored'.format(path))
        return

    try:
        with open(path, mode='r', encoding=encoding) as fin:
            text = fin.readlines()
    except UnicodeDecodeError:
        print('Failed to read file "{}"'.format(path))
        return

    rewritten_text = [regex.sub(replace_with, line) for line in text]
    if rewritten_text != text:
        print(f"Rewritten parts of {path}")
    try:
        with open(path, mode='w', encoding=encoding) as fout:
            fout.writelines(rewritten_text)
    except OSError:
        print('Failed to rewrite builder paths in "{}"!'.format(path))


def substitute_symlink(path, regex, replace_with):
    link_path = os.readlink(path)
    rewritten_link = regex.sub(replace_with, link_path)
    os.remove(path)
    os.symlink(rewritten_link, path)


def main():
    parser = ap.ArgumentParser(
        formatter_class=ap.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--fix-links', '-l', action='store_true',
                        help='Use this flag if symlinks should be rewritten')
    parser.add_argument('build_directory',
                        help='The $INSTALL_PREFIX of an external deps build.')
    parser.add_argument('search_pattern',
                        help='The text to search $INSTALL_PREFIX for')
    parser.add_argument('replace_text',
                        help='The text to replace "search_pattern" with')

    args = parser.parse_args()
    fix_paths(args.build_directory, args.search_pattern, args.replace_text,
              fix_links=args.fix_links)

if __name__ == '__main__':
    main()
