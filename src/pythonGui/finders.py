"""This is the machinery to find the virtual packages in a macro"""

import manager
import sys


class ProjectFinder(object):
    project = None


    def __init__(self, path):
        if path != projectmagic:
            raise ImportError


    def find_module(self, name):
        return self.project.macros.get(name[7:])


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
