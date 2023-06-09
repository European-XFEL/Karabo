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
from karabo.common.api import walk_traits_object
from karabo.common.project.api import MacroModel, ProjectModel


def test_project_modified_obj_check():
    modified = []

    def find_modified(obj):
        if obj.modified:
            modified.append(obj)

    macro = MacroModel(code="print('hello world')", initialized=True)
    proj = ProjectModel(macros=[macro], initialized=True)

    walk_traits_object(proj, find_modified)
    assert len(modified) == 0

    macro.code = "print('I was modified!')"
    walk_traits_object(proj, find_modified)
    assert modified == [proj, macro]
