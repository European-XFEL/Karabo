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
from qtpy.QtWidgets import QWizard, QWizardPage
from traits.api import (
    Constant, DelegatesTo, HasStrictTraits, Instance, on_trait_change)

from .pages import ConfigurePage, LinkPage, SelectScenesPage


class BasePageController(HasStrictTraits):
    # Every page has a reference to the main trait wizard
    wizard_controller = Instance(object)

    page = Instance(QWizardPage)
    needs_completion = Constant(False)
    commit_page = Constant(False)

    # Delegates
    available_scenes = DelegatesTo("wizard_controller")
    selected_scenes = DelegatesTo("wizard_controller")
    link = DelegatesTo("wizard_controller")
    project_model = DelegatesTo("wizard_controller")

    def __init__(self, **traits):
        super().__init__(**traits)
        page = self.page
        page.set_complete(not self.needs_completion)
        page.setCommitPage(self.commit_page)

    def init(self):
        """Re-implement on subclass if further processes are needed on
         initialization of page. Visiting it back doesn't trigger this anymore.
        """

    def enter(self):
        """Re-implement on subclass if actions are needed to be done when
        the page is loaded. Visiting it back triggers this."""

    def exit(self):
        """Re-implement on subclass if actions are needed to be done when
        the page is left"""


class SelectScenesController(BasePageController):
    """The scenes controller is for the validation of the selected scenes"""
    page = Instance(SelectScenesPage)
    needs_completion = Constant(True)

    # ----------------------------------------------------------------------
    # Controller methods

    def init(self):
        """Populate the list pages"""
        available = [(scene.simple_name, scene)
                     for scene in self.available_scenes]
        self.page.set_available_scenes(available)

        selected = [(scene.simple_name, scene)
                    for scene in self.selected_scenes]
        self.page.set_selected_scenes(selected)
        self._validate_selected_scenes()

    # ----------------------------------------------------------------------
    # Trait methods

    def _page_default(self):
        page = SelectScenesPage()
        # Note: For some reason on races we cannot use Qt signals here
        # Hence, use handlers.
        page.sceneAdded = self._add_scene
        page.sceneRemoved = self._remove_scene
        return page

    @on_trait_change("selected_scenes_items")
    def _validate_selected_scenes(self):
        has_selected = bool(len(self.selected_scenes))
        self.page.set_complete(has_selected)

    # ----------------------------------------------------------------------
    # Private methods

    def _add_scene(self, model):
        if model is None:
            return

        self.available_scenes.remove(model)
        self.selected_scenes.append(model)

    def _remove_scene(self, model):
        if model is None:
            return

        self.selected_scenes.remove(model)
        self.available_scenes.append(model)


class ConfigureController(BasePageController):
    """The Configure Controller provides the basic configuration options
    for the network, username, splash and login dialog"""
    page = Instance(ConfigurePage)
    needs_completion = Constant(True)
    commit_page = Constant(True)

    def init(self):
        if self.project_model is not None:
            self.page.set_subheader(
                f"Project: {self.project_model.simple_name}")

        self.link["uuids"] = [scene.uuid for scene in self.selected_scenes]
        self.page.set_config(self.link)
        self.page.set_complete(True)

    def enter(self):
        self.page.configChanged.connect(self._set_config)

    def exit(self):
        self.page.configChanged.disconnect(self._set_config)

    # ----------------------------------------------------------------------
    # Trait methods

    def _page_default(self):
        return ConfigurePage()

    # ----------------------------------------------------------------------
    # Helper methods

    def _set_config(self, config):
        self.link.update(config)


class LinkController(BasePageController):
    """The link controller is the final controller setting the LinkPage
    to generate the `url` to the clipboard"""
    page = Instance(LinkPage)

    def init(self):
        self.page.set_data(self.link)

    def enter(self):
        button = self.wizard_controller.widget.button(QWizard.FinishButton)
        button.clicked.connect(self.page.finished)

    def exit(self):
        button = self.wizard_controller.widget.button(QWizard.FinishButton)
        button.clicked.disconnect(self.page.finished)

    # ----------------------------------------------------------------------
    # Trait methods

    def _page_default(self):
        page = LinkPage()
        return page
