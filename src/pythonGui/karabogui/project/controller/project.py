#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QFont, QMenu
from traits.api import Instance, List, on_trait_change

from karabo.common.project.api import ProjectModel
from karabogui import icons
from karabogui import messagebox
from .bases import BaseProjectController
from .project_groups import ProjectSubgroupController, ProjectControllerUiData


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
        font = QFont()
        font.setBold(True)
        return ProjectControllerUiData(font=font, icon=icons.folder)

    @on_trait_change('model.is_trashed')
    def _update_icon_style(self):
        if self.model.is_trashed:
            self.ui_data.icon = icons.folderTrash
            if self.parent is not None:
                messagebox.show_warning(
                    "You are working with an actively trashed project "
                    "<b>{}</b> with uuid <b>{}</b>! Please untrash or close "
                    "the project.".format(self.model.simple_name,
                                          self.model.uuid),
                    modal=False)
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
