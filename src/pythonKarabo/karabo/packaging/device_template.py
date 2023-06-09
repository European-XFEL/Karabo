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
import os
import os.path as op
import re
import tokenize
from datetime import datetime

from karabo import version as karabo_version


def rename_files_and_directories(path, class_name, package_name):
    def rename(root, names, regex, new_name):
        replaced_indices = []
        for i, name in enumerate(names):
            if regex.search(name) is not None:
                old_path = op.join(root, name)
                new_path = op.join(root, regex.sub(new_name, name))
                os.rename(old_path, new_path)
                replaced_indices.append(i)
        for i in replaced_indices:
            names[i] = regex.sub(names[i], new_name)

    for root, dirs, files in os.walk(path, topdown=True):
        rename(root, files, re.compile('__CLASS_NAME__'), class_name)
        rename(root, dirs, re.compile('__PACKAGE_NAME__'), package_name)


def make_substitutions(path, substitutions):
    try:
        with open(path, mode='rb') as fp:
            encoding = tokenize.detect_encoding(fp.readline)[0]
    except SyntaxError:
        print(f'Unreadable file "{path}" ignored')
        return

    try:
        with open(path, encoding=encoding) as fin:
            text = fin.readlines()
    except UnicodeDecodeError:
        print(f'Failed to read file "{path}"')
        return

    rewritten_text = []
    for line in text:
        for regex, replacement in substitutions.items():
            line = regex.sub(replacement, line)
        rewritten_text.append(line)

    with open(path, mode='w', encoding=encoding) as fout:
        fout.writelines(rewritten_text)


def configure_template(path, package_name, class_name, email, template_id):
    """Configure a device template

    This renames files and directories named __CLASS_NAME__ and
    __PACKAGE_NAME__. After that, it replaces various other __WORD__-style
    template names in the files that belong to the template.
    """
    date = datetime.now().strftime("%B %d, %Y, %I:%M %p")
    substitutions = {
        re.compile('__CLASS_NAME__'): class_name,
        re.compile('__CLASS_NAME_ALL_CAPS__'): class_name.upper(),
        re.compile('__DATE__'): date,
        re.compile('__KARABO_VERSION__'): karabo_version,
        re.compile('__TEMPLATE_ID__'): template_id,
        re.compile('__EMAIL__'): email,
        re.compile('__PACKAGE_NAME__'): package_name,
        re.compile('__PACKAGE_NAME_ALL_CAPS__'): package_name.upper(),
    }

    # Do the path renames first
    rename_files_and_directories(path, class_name, package_name)

    for dirpath, _, filenames in os.walk(path):
        for name in filenames:
            file_path = op.join(dirpath, name)
            make_substitutions(file_path, substitutions)
