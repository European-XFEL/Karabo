#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 27, 2017
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
#############################################################################
from traits.api import Bool, Event, HasStrictTraits, Instance, List, Property

from karabo.common.api import walk_traits_object
from karabo.common.project.api import BaseProjectObjectModel, ProjectModel


class ProjectLoadingWatcher(HasStrictTraits):
    """An object which can be set as a traits notification handler for projects
    which are loading asynchronously from the database.
    """
    # A project to be loaded
    project = Instance(ProjectModel)
    # A property which can be checked by external code
    finished = Property(Bool)
    # When this event fires, the project is loaded!
    finished_event = Event

    # Project objects which are not yet loaded
    _loading_objects = List(Instance(BaseProjectObjectModel))

    def _get_finished(self):
        return len(self._loading_objects) == 0

    def _project_changed(self):
        if self.project is not None:
            self._watch_loading_object(self.project)

    def _notification_handler(self, obj, name, old, new):
        """Traits notification handler for loading objects.
        """
        if new:
            self._loading_objects.remove(obj)
            obj.on_trait_change(self._notification_handler, 'initialized',
                                remove=True)

            # It's possible that this object introduced more children which
            # now need to be watched
            self._watch_loading_object(obj)

        # Are we done yet?
        if self.finished:
            self.finished_event = True

    def _watch_loading_object(self, project_object):
        """Add a whole bunch of traits listeners to a project object and its
        children so that it can be determined when all of its children are
        loaded.
        """
        def visitor(model):
            if isinstance(model, BaseProjectObjectModel):
                if not model.initialized:
                    model.on_trait_change(self._notification_handler,
                                          'initialized')
                    self._loading_objects.append(model)

        walk_traits_object(project_object, visitor)
