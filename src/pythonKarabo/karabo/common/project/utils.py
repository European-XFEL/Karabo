from .model import ProjectModel, visit_project_objects


def find_parent_project(model, root_project):
    """ Given a project child object and a project model which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param root_project: A ProjectModel which is the ancestor of ``model``.
    """
    class _Visitor(object):
        last_project = None
        parent = None

        def __call__(self, obj):
            if obj is model:
                if self.parent is not None:
                    msg = "Object {} is in the project more than once!"
                    raise RuntimeError(msg.format(obj))
                self.parent = self.last_project
            if isinstance(obj, ProjectModel):
                self.last_project = obj

    visitor = _Visitor()
    visit_project_objects(root_project, visitor)
    return visitor.parent
