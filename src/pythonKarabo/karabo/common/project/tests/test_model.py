from karabo.common.project.api import (
    ProjectModel, MacroModel, visit_project_objects)


def test_project_modified_obj_check():
    modified = []

    def find_modified(obj):
        if obj.modified:
            modified.append(obj)

    macro = MacroModel(code="print('hello world')")
    proj = ProjectModel(macros=[macro])

    visit_project_objects(proj, find_modified)
    assert len(modified) == 0

    macro.code = "print('I was modified!')"
    visit_project_objects(proj, find_modified)
    assert modified == [macro]
