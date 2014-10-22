"""This is the machinery to find the virtual packages in a macro"""

import manager
import sys

from importlib.machinery import ModuleSpec


class ProjectFinder(object):
    project = None


    def __init__(self, path):
        if path != projectmagic:
            raise ImportError


    def find_spec(self, name, target):
        if target is None:
            s = ModuleSpec(name, self.project.macros.get(name[7:]),
                           origin=self.project.filename + "/macros/" +
                           name[7:])
            s.has_location = True
            return s
        else:
            return target.__spec__


projectmagic = "some random text for projects"
sys.path_hooks.append(ProjectFinder)


class MacroContext(object):
    def __init__(self, project):
        self.project = project


    def __enter__(self):
        ProjectFinder.project = self.project
        sys.modules.update(self.project.modules)


    def __exit__(self, a, b, c):
        ProjectFinder.project = None
        self.project.modules = {k: v for k, v in sys.modules.items()
                                if k.startswith("macros.")}
        for k in self.project.modules:
            del sys.modules[k]
