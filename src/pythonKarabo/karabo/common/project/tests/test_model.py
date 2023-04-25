# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
