#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import HasStrictTraits, Bool, Event, Instance, Property

from karabo.middlelayer import Hash
from karabo_gui.configuration import Configuration
from .util import clear_configuration_instance


class ProjectDeviceInstance(HasStrictTraits):
    """An abstraction of devices needed for projects and workflow design.
    No matter the online/offline state of a device, you can get a
    ``Configuration`` object which is valid and associated with the same
    device id.
    """
    # The current configuration for this device
    current_configuration = Property(Instance(Configuration))

    # An event which is triggered whenever the class schema changes
    schema_updated = Event

    # An event which is triggered whenever the configuration is updated
    configuration_updated = Event

    # Binary online/offline state of the device
    online = Bool

    # The online/offline configurations
    _initial_configuration = Instance(Hash)
    _class_config = Instance(Configuration)
    _project_device = Instance(Configuration)
    _real_device = Instance(Configuration)

    def __init__(self, real_device, project_device, class_config, init_config):
        super(ProjectDeviceInstance, self).__init__()

        self.online = real_device.isOnline()
        self._initial_configuration = init_config
        self._class_config = class_config
        self._project_device = project_device
        self._real_device = real_device

        # Connect to signals
        class_config.signalNewDescriptor.connect(self._descriptor_change_slot)
        real_device.signalStatusChanged.connect(self._status_change_slot)
        real_device.signalBoxChanged.connect(self._config_change_slot)
        project_device.signalBoxChanged.connect(self._config_change_slot)

        if class_config.descriptor is not None:
            self._descriptor_change_slot(class_config)

    def destroy(self):
        """Disconnect slots which are connected to this object's methods
        """
        new_descriptor = self._descriptor_change_slot
        self._class_config.signalNewDescriptor.disconnect(new_descriptor)
        status_changed = self._status_change_slot
        self._real_device.signalStatusChanged.disconnect(status_changed)
        config_changed = self._config_change_slot
        self._real_device.signalBoxChanged.disconnect(config_changed)
        self._project_device.signalBoxChanged.disconnect(config_changed)

        clear_configuration_instance(self._project_device)

    def set_offline_configuration(self, configuration):
        """Forcibly set the offline configuration Hash of the device.
        """
        if self._project_device.descriptor is None:
            return

        self._project_device.setDefault()
        self._project_device.fromHash(configuration)

    # ---------------------------------------------------------------------
    # Traits Handlers

    def _get_current_configuration(self):
        """Traits Property getter for the current configuration
        """
        if self.online:
            return self._real_device

        return self._project_device

    # ---------------------------------------------------------------------
    # Qt Slots

    def _config_change_slot(self):
        """The (possibly) current ``Configuration`` object has been edited by
        a user.
        """
        # Let the world know
        self.configuration_updated = True

    def _descriptor_change_slot(self, config):
        """The global class has received a new schema which needs to be set
        for the dependent device ``Configuration`` instances.
        """
        if self._project_device.descriptor is not None:
            self._project_device.redummy()
        self._project_device.descriptor = config.descriptor

        if self._real_device.descriptor is not None:
            self._real_device.redummy()
        self._real_device.descriptor = config.descriptor

        # Set values for offline configuration
        self._project_device.setDefault()
        if self._initial_configuration is not None:
            self._project_device.fromHash(self._initial_configuration)

        # Let the world know
        self.schema_updated = True

    def _status_change_slot(self, status, error_flag):
        """The `_real_device` has changed its status. Check if it's online
        """
        self.online = self._real_device.isOnline()
