#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtWidgets import QMenu
from traits.api import Instance, List, on_trait_change

from karabo.common.project.api import ProjectModel
from karabogui import icons, messagebox
from karabogui.enums import ProjectItemTypes
from karabogui.fonts import get_qfont

from .bases import BaseProjectController
from .project_groups import ProjectControllerUiData, ProjectSubgroupController


class ProjectController(BaseProjectController):
    """ A controller for ProjectModel objects
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The subgroups of this project
    children = List(Instance(ProjectSubgroupController))

    def context_menu(self, project_controller, parent=None):
        return QMenu(parent)

    def create_ui_data(self):
        font = get_qfont()
        font.setBold(True)
        return ProjectControllerUiData(font=font, icon=icons.folder)

    @on_trait_change('model.conflict')
    def _update_conflict_icon(self):
        if self.model.conflict:
            self.ui_data.conflict_icon = icons.alarmGlobal

    @on_trait_change('model.is_trashed')
    def _update_icon_style(self):
        if self.model.is_trashed:
            self.ui_data.icon = icons.folderTrash
            if self.parent is not None:
                messagebox.show_warning(
                    "You are working with an actively trashed project "
                    "<b>{}</b>! Please untrash or close "
                    "the project.".format(self.model.simple_name))
        else:
            self.ui_data.icon = icons.folder

    def child(self, index):
        """Returns a child of this controller.

        :param index: An index into the list of this controller's children
        :returns: A BaseProjectController instance or None
        """
        return self.children[index]

    def rows(self):
        """Returns the number of rows 'under' this controller in the project
        tree view.
        """
        return len(self.children)

    def info(self):
        return {'type': ProjectItemTypes.PROJECT}
