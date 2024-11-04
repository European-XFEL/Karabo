# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtWidgets import QWidget
from traits.api import Any, Dict, HasStrictTraits, Instance, List

from karabo.common.project.api import ProjectModel
from karabogui.singletons.api import get_config, get_network

from .controllers import (
    ConfigureController, LinkController, SelectScenesController)
from .pages import PageContainer


class CinemaWizardController(HasStrictTraits):
    # The QWizard PageContainer
    widget = Instance(PageContainer)

    # Parent widget, can be None
    parent = Instance(QWidget, transient=True)

    # Shared variables
    project_model = Instance(ProjectModel)
    available_scenes = List
    selected_scenes = List
    link = Dict

    # Pages
    page_controller = List
    current_controller = Any

    def run(self):
        """Run the cinema wizard for the different entry points"""
        if len(self.selected_scenes):
            # Started directly on a scene
            self._set_start_page(ConfigureController)
        elif self.project_model is not None:
            # Started for the scene group
            self._set_start_page(SelectScenesController)
            self.available_scenes = self.project_model.scenes

        self.widget.show()

    # ----------------------------------------------------------------------
    # Trait methods

    def _widget_default(self):
        wizard = PageContainer(parent=self.parent)
        # Add page_controller
        for index, controller in enumerate(self.page_controller):
            wizard.setPage(index, controller.page)
        # Connect signals
        wizard.currentIdChanged.connect(self._set_current_controller)

        return wizard

    def _page_controller_default(self):
        return [
            SelectScenesController(wizard_controller=self),
            ConfigureController(wizard_controller=self),
            LinkController(wizard_controller=self)]

    def _link_default(self):
        network = get_network()
        return dict(
            domain=get_config()["domain"],
            host=network.hostname,
            port=network.port,
            include_host=False,
            show_splash=True,
            uuids=[])

    def _current_controller_changed(self, old, new):
        if old is not None:
            old.exit()
        if new is not None:
            # Check if page is already visited
            if not self._has_visited(old, new):
                new.init()
            new.enter()

    # ----------------------------------------------------------------------
    # Private methods

    def _set_current_controller(self, index):
        self.current_controller = self.page_controller[index]

    def _set_start_page(self, klass):
        for index, page in enumerate(self.page_controller):
            if isinstance(page, klass):
                self.widget.setStartId(index)
                return

        raise IndexError(f"No page_controller of class {klass} found.")

    def _has_visited(self, old, new):
        def get_id(page):
            page_id = -1
            if page is not None:
                page_id = self.page_controller.index(page)
            return page_id

        return get_id(new) < get_id(old)
