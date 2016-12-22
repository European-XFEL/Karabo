.. _project_conf:

************************
Projects & Configuration
************************

.. todo::

	For this chapter I've assumed that we at some point also use the project view and
	configuration to configure and execute device deployment. This is optional functionality
	for day-1 but in my point of view is a natural consequence of having devices bound
	to device servers and having a host property on the server. We should keep this in
	mind, in terms what information is stored with device servers, e.g. Karabo version
	number etc, to then implement a deployment device at a later time.

Device Configuration
====================

Device configuration is handled in so called projects, which are versioned in a 
central repository. Projects are domain-bound, i.e. a project configuration is always
pertinent to the domain it is employed in. Project configurations may however, if they
are marked as global, be shared across domains: the configuration will be
bound to the new domain it is loaded in. 

This mechanism allows for a consistent central versioning of project which are used in
different places, i.e. detectors which may be moved between hutches and the detector lab
but should have the same or very similar configurations in all locations.

Projects hold either device servers, or subprojects. Through this hierarchy subprojects
may be updated independent of the overall control system project, e.g. an instrument
project will consist of subprojects for it vacuum systems, sample delivery system and 
detectors. Subprojects may thus be used to aggregate over the functional, logical or
physical representation of a group of devices. Subprojects should however not be used 
as a type of permanent filter, e.g. by device class. This can be done via the filtering
mechanisms of the project view.

Configurations may be persisted as part of three procedures:

- persisting as part of a default/expert configuration. These configurations may be named
  and each named configuration is versioned independently. Default/expert configurations
  should be known working configurations, which can be reapplied to a device or device
  server if its current configuration is faulty.
  
- persisting as part of a user configuration. These configurations may be named
  and each named configuration is versioned independently. User configurations may be 
  created during operation, but are more specific to a certain beam-time. They are not
  needed to maintain general device functionality, but can be upgraded to a default/expert
  configuration if needed. User configurations should be overwritten by default
  configurations if they are faulty.
  
- persisting the last-known working configuration. If the device instance or server shuts
  down for any reason this is the configuration which will be loaded if not otherwise
  defined. They may either be saved by user action, from the GUI or the CLI (see below),
  or by default as configured in the project, device server and device server properties.
  In the latter case persisting occurs when a device instance is killed.

.. warning::

	In Karabo the persisted configurations are not automatically updated whenever a 
	property of a device is altered. You need to instead use one of the mentioned
	persisting options.
	
.. ifconfig:: includeDevInfo is True

	Device configurations are the leaf elements of the project configuration. They are
	always self-contained and complete. This means that a configuration by itself is
	sufficient to instantiate a device instance of a given device class. While logically
	the device server is the node above the device configuration, i.e. the instance
	representing leaf, in the backend-implementation up to two hierarchy levels are 
	foreseen. 
	
    .. graphviz::
        :caption: Lowest hierarchy level of the configuration and project management.
            Folder icons denote hierarchy levels in the eXistDB organization, boxes XML
            file representing named configurations. The database handles versioning of
            each named configuration.

        digraph instance_config {

            rankdir = LR;



            device_server
            [
                shape = folder
                label = "deviceServerA"
            ]

            device_a
            [
                shape = folder
                label = "deviceA"
            ]

            device_b
            [
                shape = folder
                label = "deviceB"
            ]

            device_n
            [
                shape = folder
                label = "deviceN"
            ]

            default
            [
                shape = folder
                label = "default"
            ]

            user
            [
                shape = folder
                label = "user"
            ]

            user_1
            [
                shape = folder
                label = "beamtimeA"
            ]

            user_2
            [
                shape = folder
                label = "test1"
            ]

            user_n
            [
                shape = folder
                label = "test2"
            ]



            instance
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="powderblue">
                                <tr><td colspan="3" bgcolor="beige">deviceN_confName1.xml</td></tr>
                                <tr><td colspan="3">versions</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="instance_id">instance_id</td></tr>
                                <tr><td port="class_id">class_id</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="device_version">device_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="properties">properties</td></tr>
                                <tr><td port="properties_c">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">2</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="instance_id">instance_id</td></tr>
                                <tr><td port="class_id">class_id</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="device_version">device_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="properties">properties</td></tr>
                                <tr><td port="properties_c">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">..</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="instance_id">instance_id</td></tr>
                                <tr><td port="class_id">class_id</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="device_version">device_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="properties">properties</td></tr>
                                <tr><td port="properties_c">...</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            instance_mini_2
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="powderblue">
                                <tr><td colspan="3" bgcolor="beige">deviceN_confName2.xml</td></tr>
                                <tr><td colspan="3">versions</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">2</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">..</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            instance_mini_n
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="powderblue">
                                <tr><td colspan="3" bgcolor="beige">deviceN_confNameN.xml</td></tr>
                                <tr><td colspan="3">versions</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">2</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">..</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            instance_micro
            [
                shape = box
                style = filled
                fillcolor = "powderblue"
                label = "°°°.xml"
            ]

            instance_micro_2
            [
                shape = box
                style = filled
                fillcolor = "powderblue"
                label = "°°°.xml"
            ]


            device_server -> device_a
            device_server -> device_b
            device_server -> device_n

            device_n -> default
            device_n -> user

            default -> instance
            default -> instance_mini_2
            default -> instance_mini_n

            user -> user_1
            user -> user_2
            user -> user_n
            user_n -> instance_micro
            user_n -> instance_micro_2




        }

    Additionally, devices maintain two *last known working* configurations
    not bound to any project but to the domain.

    .. graphviz::

        digraph instance_config {

            rankdir = LR;


            domain
            [
                shape = folder
                label = "domainA"
            ]


            device_server
            [
                shape = folder
                label = "deviceServerA"
            ]

            device_serverB
            [
                shape = folder
                label = "deviceServerB"
            ]

            device_serverN
            [
                shape = folder
                label = "deviceServerN"
            ]

            device_a
            [
                shape = folder
                label = "deviceA"
            ]

            device_b
            [
                shape = folder
                label = "deviceB"
            ]

            device_n
            [
                shape = folder
                label = "deviceN"
            ]



            instance_micro_lkw_in
            [
                shape = box
                style = filled
                fillcolor = "powderblue"
                label = "lkw_in.xml"
            ]

            instance_micro_lkw_out
            [
                shape = box
                style = filled
                fillcolor = "powderblue"
                label = "lkw_out.xml"
            ]

            domain -> device_server
            domain -> device_serverB
            domain -> device_serverN

            device_server -> device_a
            device_server -> device_b
            device_server -> device_n


            device_n -> instance_micro_lkw_in
            device_n -> instance_micro_lkw_out


        }

    These are updated upon successful initialization of a device (``in``) and
    upon successful shutdown of a device instance (``out``). In either case
    as they are not project bound, a device used as part of one project and
    then used again as part of another can be configured to the last
    configuration it was running in as part of the first project by using
    one of the last known working configurations.


    XML files denoting the device configurations thus have the following structure.

    .. code-block:: xml

        <name>configuratioName</name>
        <instance_id>instanceId</instance_id>
        <class_id>classId</class_id>
        <author>author</author>
        <!-- the device version is the published version of the device -->
        <device_version>deviceVersion</device_version>
        <!-- the karabo version is the karabo version this configuration was created on -->
        <krb_version>krbVersion</krb_version>

        <!-- the configuration this configuration was initially copied from, empty if none -->
        <copied_from_name>copiedFromConfigName</copied_from_name>
        <copied_from_version>copiedFromConfigVersion</copied_from_version>

        <!-- filled if a push update was requested, if non-empty acknowledgement is requested
             then emptied again upon acknowledgment. The copied_from fields get updated to this
             then -->
        <request_update_to_name>requestUpdateFromConfigName</request_update_to_name>
        <request_update_to_version>requestUpdateFromConfigName</request_update_to_version>
        <request_initiated_by>userName</request_initiated_by>
        <request_acknowledged_by>userName</request_acknowledged_by>

        <server_id>serverIf</server>

        <run_level>runLevel</run_level>

        <properties>
            <krb_property> ... </krb_property>
            <krb_property> ... </krb_property>
        </properties>


    On the next highest hierarchy level the device server is described, which can maintain
    different configurations via named, versioned snapshots.

    .. graphviz::
        :caption: Middle hierarchy level of the configuration and project management.
            Folder icons denote hierarchy levels in the eXistDB organization, blue boxes XML
            file representing named server configuration. The database handles versioning of
            each named configuration. The yellow box is an example of tag entries which would
            be found in the snapshots tag of the server configuration file. Organization
            of all server configurations in a single *servers* folder has access performance
            benefits.

         digraph server_config {

            rankdir = LR;

            project_a
            [
                shape = folder
                label = "projectA"

            ]

            server_a
            [
                shape = folder
                label = "deviceServerA"

            ]

            server_b
            [
                shape = folder
                label = "deviceServerB"

            ]

            server_n
            [
                shape = folder
                label = "deviceServerN"

            ]

            servers
            [
                shape = folder
                label = "servers"

            ]

            server_n_conf
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="powderblue">
                                <tr><td colspan="3" bgcolor="beige">serverN_confName1.xml</td></tr>
                                <tr><td colspan="3">versions</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="server_name">server_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="server_version">server_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_c1">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="server_name">server_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="server_version">server_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_c2">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="server_name">server_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="server_version">server_version</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_cn">...</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            snapshot_n
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="khaki">
                                <tr><td colspan="3" bgcolor="beige">snapshotName</td></tr>
                                <tr><td colspan="3">instances</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="id">device_id</td></tr>
                                <tr><td port="config_path">config_path</td></tr>
                                <tr><td port="config_name">config_name</td></tr>
                                <tr><td port="config_version">config_version</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="id">device_id</td></tr>
                                <tr><td port="config_path">config_path</td></tr>
                                <tr><td port="config_name">config_name</td></tr>
                                <tr><td port="config_version">config_version</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="id">device_id</td></tr>
                                <tr><td port="config_path">config_path</td></tr>
                                <tr><td port="config_name">config_name</td></tr>
                                <tr><td port="config_version">config_version</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            device_a
            [
                shape = folder
                label = "deviceA"
            ]

            device_b
            [
                shape = folder
                label = "deviceB"
            ]

            device_n
            [
                shape = folder
                label = "deviceN"
            ]

            default
            [
                shape = folder
                label = "default"
            ]

            user
            [
                shape = folder
                label = "user"
            ]


            lkw
            [
                shape = folder
                label = "last known working"
            ]

            project_a -> server_a
            project_a -> server_b
            project_a -> server_n
            project_a -> servers
            servers -> server_n_conf
            server_n_conf:snapshots_cn -> snapshot_n
            server_a -> device_a
            server_a -> device_b
            server_a -> device_n
            device_n -> default
            device_n -> user
            device_n -> lkw


        }


    XML files denoting the server configurations thus have the following structure.

    .. code-block:: xml

        <name>configuratioName</name>
        <server_name>serverName</server_name>
        <author>author</author>

        <!-- the server version is the published version of the server -->
        <server_version>serverVersion</server_version>

        <!-- the karabo version is the karabo version this configuration was created on -->
        <krb_version>krbVersion</krb_version>

        <!-- the configuration this configuration was initially copied from, empty if none -->
        <copied_from_name>copiedFromConfigName</copied_from_name>
        <copied_from_version>copiedFromConfigVersion</copied_from_version>

        <!-- filled if a push update was requested, if non-empty acknwoledgement is requested
             then emptied again upon acknowledgment. The copied_from fields get updated to this
             then -->
        <request_update_to_name>requestUpdateFromConfigName</request_update_to_name>
        <request_update_to_version>requestUpdateFromConfigName</request_update_to_version>
        <request_initiated_by>userName</request_initiated_by>
        <request_acknowledged_by>userName</request_acknowledged_by>

        <!-- snapshots section -->
        <snapshots>
            <snapshot name="snapshotA" createdAt="30032016T143500">
                <instance>
                    <device_id>deviceId</device_id>
                    <config_path>default</config_path>
                    <config_name>configName</config_name>
                    <config_version>1.2</config_version>
                </instance>
                <instance>
                    <device_id>deviceId</device_id>
                    <config_path>users.beamtimeA</config_path>
                    <config_name>configName</config_name>
                    <config_version>1.2</config_version>
                </instance>
                ...
            </snapshot>
            <snapshot name="snapshotB" createdAt="31032016T143500">
            ...
            </snapshot>
        </snapshots>


    The final, and topmost hierarchy level is the project level.

    .. graphviz::
        :caption: Top hierarchy level of the configuration and project management.
            Folder icons denote hierarchy levels in the eXistDB organization, blue boxes XML
            file representing named project configuration. The database handles versioning of
            each named configuration. The yellow box is an example of tag entries which would
            be found in the snapshots tag of the project configuration file. Organization
            of all project configurations in a single *project* folder has access performance
            benefits. The project snapshots also resolve to the members of this project, which
            can either be projects, i.e. sub-projects or servers.

        digraph server_config {

            rankdir = LR;

            domain_a
            [
                shape = folder
                label = "domainA"

            ]

            project_a
            [
                shape = folder
                label = "projectA"

            ]

            project_b
            [
                shape = folder
                label = "projectB"

            ]

            project_n
            [
                shape = folder
                label = "projectN"

            ]

            projects
            [
                shape = folder
                label = "projects"

            ]

            server_a
            [
                shape = folder
                label = "deviceServerA"

            ]

            server_b
            [
                shape = folder
                label = "deviceServerB"

            ]

            server_n
            [
                shape = folder
                label = "deviceServerN"

            ]

            servers
            [
                shape = folder
                label = "servers"

            ]

            project_n_conf
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="powderblue">
                                <tr><td colspan="3" bgcolor="beige">projectN_confName1.xml</td></tr>
                                <tr><td colspan="3">versions</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="version">1</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="project_name">project_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_c1">...</td></tr>
                                <tr><td port="members">members</td></tr>
                                <tr><td port="members_c1">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">2</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="project_name">project_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_c2">...</td></tr>
                                <tr><td port="members">members</td></tr>
                                <tr><td port="members_c2">...</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="version">N</td></tr>
                                <tr><td port="name">name</td></tr>
                                <tr><td port="project_name">project_name</td></tr>
                                <tr><td port="author">author</td></tr>
                                <tr><td port="krb_version">krb_version</td></tr>
                                <tr><td port="snapshots">snapshots</td></tr>
                                <tr><td port="snapshots_cn">...</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]

            snapshot_n
            [
                shape = none
                label = <<table border="1" cellspacing="0" bgcolor="khaki">
                                <tr><td colspan="3" bgcolor="beige">snapshotName</td></tr>
                                <tr><td colspan="3">servers</td></tr>
                                <tr><td><table border="0" cellspacing="0">
                                <tr><td port="id">member_name</td></tr>
                                <tr><td port="id">member_type</td></tr>
                                <tr><td port="snapshot_name">snapshot_name</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="id">member_name</td></tr>
                                <tr><td port="id">member_type</td></tr>
                                <tr><td port="snapshot_name">snapshot_name</td></tr>
                                </table></td>
                                <td><table border="0" cellspacing="0">
                                <tr><td port="id">member_name</td></tr>
                                <tr><td port="id">member_type</td></tr>
                                <tr><td port="snapshot_name">snapshot_name</td></tr>
                                </table></td>
                                </tr>
                        </table>>
            ]


            device_a
            [
                shape = folder
                label = "deviceA"
            ]

            device_b
            [
                shape = folder
                label = "deviceB"
            ]

            device_n
            [
                shape = folder
                label = "deviceN"
            ]


            domain_a -> project_a
            domain_a -> project_b
            domain_a -> project_n
            domain_a -> projects
            project_n -> server_a
            project_n -> server_b
            project_n -> server_n
            project_n -> servers
            projects -> project_n_conf
            project_n_conf:snapshots_cn -> snapshot_n
            server_a -> device_a
            server_a -> device_b
            server_a -> device_n



        }


	XML files denoting the project configurations thus have the following structure.

    .. code-block:: xml

        <name>configuratioName</name>
        <project_name>projectName</project_name>
        <author>author</author>
        <!-- the karabo version is the karabo version this configuration was created on -->
        <krb_version>krbVersion</krb_version>

        <!-- the configuration this configuration was initially copied from, empty if none -->
        <copied_from_name>copiedFromConfigName</copied_from_name>
        <copied_from_version>copiedFromConfigVersion</copied_from_version>

        <!-- filled if a push update was requested, if non-empty acknowledgement is requested
             then emptied again upon acknowledgment. The copied_from fields get updated to this
             then -->
        <request_update_to_name>requestUpdateFromConfigName</request_update_to_name>
        <request_update_to_version>requestUpdateFromConfigName</request_update_to_version>
        <request_initiated_by>userName</request_initiated_by>
        <request_acknowledged_by>userName</request_acknowledged_by>

        <!-- snapshots section -->
        <snapshots>
            <snapshot name="snapshotA" createdAt="30032016T143500">
                <member>
                    <member_name>serverA</member_name>
                    <member_type>server</member_type>
                    <snapshot_name>snapshotA</snapshot_name>
                </member>
                <member>
                    <member_name>projectB</member_name>
                    <member_type>project</member_type>
                    <snapshot_name>snapshotC</snapshot_name>
                </member>
                ...
            </snapshot>
            <snapshot name="snapshotB" createdAt="31032016T143500">
            ...
            </snapshot>
        </snapshots>

    Snapshot resolution propagates down the hierarchy, i.e. selecting a project snapshot will lead
    to it looking at its member's correspond snapshot names, these in turn resolve the snapshots for
    the next hierarchy level (in case of a sub-project member), or the configuration name and version,
    in case of a device server member.

    .. note::

        Configuration resolution should happen by the database. The project manager service
        responsible for device instantiation should not have to be aware of the hierarchy and
        configuration resolution. Instead the database service should resolve all queries to a single
        XML-based configuration file for a given device instance (the leaf elements in the
        database) and created a local copy of these in a flat hierarchy. The project manager
        then instantiates the device ids from these configuration files.

        The GUI however needs to be aware of the hierarchy to a certain extend, in order to render
        the hierarchical project/subproject, project/server/instance structure accordingly.
	
Configuring Last-Known-Working Configurations
+++++++++++++++++++++++++++++++++++++++++++++

Apart from user-driven saving of the last known working configuration each project entity,
i.e. projects, sub-projects, device servers, devices offer a configuration field if the 
devices below this hierarchy level should persist their last known configuration as part
of their shutdown procedure. The of this parameter happens top-down through the hierarchy:
it may either be set to *yes*, *no* or *parent*.

Project & Server Snapshots
++++++++++++++++++++++++++

Servers can be requested to snapshot the configurations their instances currently use
as part of a named snapshot.

Similarly, projects may be snapshot on each hierarchy level, i.e. record the named snaphots
subprojects or servers contained in them currently use. 

.. note::

	Upon creating a server or a project a default snapshot (named "default") is created.
	This snapshot is updated whenever you trigger snapshot creation and have created a
	named snapshot yet. You may also at anytime update all default snapshots by the 
	corresponding commands.

Device Groups
=============

A device may be loaded into the device server hosting it as a device group. In such a 
case many similar instances of the device will be created, all sharing the initial group
configuration. Each instance may be afterwards individually configured, overwriting the
predefined group configuration. If necessary, all devices in the group may be reset to
the group configuration.

On the group configuration level the user may specify simple batch changes to the devices
in the group:

- The number of instance in the group to be created, allowing also for addition of further
  instances at a later time.
- Autogeneration of the instance ID out of a prefix and a counting index suffixed to this.
- For scalar fields substitution by a counting variable, with definable start index and
  increment is possible.
- For scalar and string fields: assignment out of a value list, by modulo of the device
  index.
- For input channels, connection of all devices in a group to all output channels specified
  in a list (scatter topology)
- For output channels, connection of all devices to a group of input channels specified
  in a list (gather topology)

Consider having a controller that exposes many channels of a power supply in
a single *driver* device. Each channels properties are grouped in a
``NODE_ELEMENT`` which has the key *channel_n* where *n* is an number running
from 0 to N channels.

.. code-block:: Python

    class MultiChannelController(PythonDevice):

        def expectedParameters(expected):
            (
                NODE_ELEMENT(expected).key("channel_0")
                    .commit()
                    ,
                FLOAT_ELEMENT(expected).key("channel_0.voltage")
                    .readOnly()
                    .commit()
                    ,
                FLOAT_ELEMENT(expected).key("channel_0.current")
                    .readOnly()
                    .commit()
                    ,
                SLOT_ELEMENT(expected).key("channel_0.channel_0_ramp")
                    .displayedName("Ramp")
                    ,
                NODE_ELEMENT(expected).key("channel_1")
                    .commit()
                    ,
                FLOAT_ELEMENT(expected).key("channel_1.voltage")
                    .readOnly()
                    .commit()
                    ,
                FLOAT_ELEMENT(expected).key("channel_1.current")
                    .readOnly()
                    .commit()
                    ,
                SLOT_ELEMENT(expected).key("channel_0.channel_0_ramp")
                    .displayedName("Ramp")
                    ,
                #...

            )

To expose each device as a separate middle-layer device you would write a
device template

.. code-block:: Python

    class SingeChannelDevice(Device):

       channel = DeviceNode(displayedName = "channel")

       controllerId = String(displayedName = "Controller id")
       nodeIdOnController = String(displayedName = "Channel name")

       def __init__(self):
          #node specifies lowest hierarchy level
          self.channel.node = self.nodeIdOnController
          self.channel.properties=["voltage", "current"]
          self.channel.commands = ["{}_ramp".format(self.nodeIdOnController,
                                    "ramp")]
          self.channel.instance = self.controllerId
          self.channel.connect()

In the device group configuration you would now specify to automatically fill
the ``nodeIdOnController`` property as "channel\_"+*#INSTANCE*, where
*#INSTANCE* runs from 0 to N. The ``controller`` id should be automatically
set to the same controller instance id for all devices in the group.

Each ``SingleChannelDevice`` will then contain the voltage and current
values for the channel identified by channel name in a flat hierarchy.

Device Server Groups
====================

Similar to device groups, device server groups may be specified. These are then distributed
to a list of hosts.

On the group configuration level the user may specify simple batch changes to the devices
in the group:

- The number of server instances in the group to be created, allowing also for addition 
  of further instances at a later time.
- Autogeneration of the instance ID out of a prefix and a counting index suffixed to this.
- For input channels, connection of all devices in a group to all output channels specified
  in a list (scatter topology)
- For output channels, connection of all devices to a group of input channels specified
  in a list (gather topology)
- Input/Output channels in a device server group can be configured to be the same for each
  server in the group. I.e. if the template for the server group specifies that the output
  of device A connects to the input of device B, which in turn connects its output to the
  input of device C, all device servers will have the A->B->C connection schema applied.
  Additionally, one might then configure a scatter operation to the inputs of the A devices
  and a gather from the outputs of the C devices, yielding the below graph.
  
  .. digraph:: device_server_group_sg

		"PROC_S" -> "PROC0_A";
		"PROC0_A" -> "PROC0_B";
		"PROC0_B" -> "PROC0_C";
		"PROC0_C" -> "PROC_G";
		
		"PROC_S" -> "PROC1_A";
		"PROC1_A" -> "PROC1_B";
		"PROC1_B" -> "PROC1_C";
		"PROC1_C" -> "PROC_G";
		
		"PROC_S" -> "PROCN_A";
		"PROCN_A" -> "PROCN_B";
		"PROCN_B" -> "PROCN_C";
		"PROCN_C" -> "PROC_G";
		

  
The device server groups are especially useful for pipelined processing scenarios, where
the parallel parts of the pipeline would be configured once for a device server, and this
would be instantiated many times as part of a device server group.



Run Levels
==========

Devices and device groups can be configured to one of seven run-levels, stepped through
in sequence: 

============= =============================================================================
**Run level** **To be used for**
------------- -----------------------------------------------------------------------------
CORE	      Core karabo devices, which need to be up and running before all other devices
SYSTEM        Karabo system devices, which need instantiated core devices to work
INTERFACE     Communication interface devices, e.g. interface to crates
MANAGER       Manager-type devices, which might rely on subsequent devices to register with them
DRIVER        Hardware driving devices on which middle-layer devices work upon
MIDDLE_LYR_0  First level of middle layer devices
MIDDLE_LYR_1  Second level of middle layer devices
============= =============================================================================

Instantiation happens in top to bottom sequence for the above table. Similarly instances
are killed in the reverse order, i.e. bottom to top.

Run-level consistency is guaranteed up to the encapsulating project level, i.e. for the
all device servers on the same project hierarchy. It is not guaranteed device servers
residing in parent- or sub-projects. 

.. todo::

	Clarify concept, if number of levels suffices, and if the encapsulation policy is to
	restrictive. Implement first on the project level side, i.e. no back-end protection 
	for out of project instantiations
	
	To discuss concept with: Burkhard, Gero, USPs, Patrick
	To discuss implementation with: Martin, Sergey, Kerstin (config through GUI)
	
The following table gives a few examples:

============= =============================================================================
**Run level** **Examples**
------------- -----------------------------------------------------------------------------
CORE	      Core deployment device, Core configuration device, Auth device, Gui Server
SYSTEM        Data logger, run configuration device,
INTERFACE     BeckhoffCom, MpodCrate, CalDBRemote, CalDBLocal
MANAGER       CalibrationManager, 
DRIVER        BeckhoffMotor, BeckhoffGuage, 
MIDDLE_LYR_0  MpodChannel Devices
MIDDLE_LYR_1  Scan Devices, Slit Devices, MpodChannel Groups
============= =============================================================================

The Central Configuration Database
==================================

The central configuration database maintains configurations and versioning information
there-of. It usually resolves configurations by domain, but users may load configured
device servers from a global repository as well. Aside from single instance configuration
the following "batch" jobs may be requested through the database:

- Apply a new configuration either in full or for selected expected parameters to all
  device instances of a specific class at downwards of a specified domain hierarchy level.
- Duplicate an existing (expert) configuration of a device server from the global repository
  to become part of a project at a given domain level and then maintain and version this
  configuration separately. In this case it is expected to give the new configuration 
  a unique name. The new configuration will keep track of from which configuration it was
  copied from.
- Include preconfigured (sub)-projects from the global or domain specific repository and
  use them. Project elements such as scenes and resource data also maintain knowledge of
  from where they were copied and as such may receive an update if the master resource
  has been updated. 
  
In all cases the availability of an update due to the master resource having changed it
indicated in the GUI or on the CLI. Updates need to be individually, or batch acknowledged
to take effect. 

Command Line Interface (iKarabo)
================================

The following project related commands are available from the iKarabo interface.

Inspection Routines
+++++++++++++++++++

.. function:: listProjects(string domain = iKarabo.currentDomain)

   returns a list of projects at the given domain level, subdomains are seperated
   by slashes ("/"). The representation of the returned list of strings is such
   that it renders to a list with description of the project.
   
.. function:: listDeviceServers(string domain = iKarabo.currentDomain)

   returns a list of all device servers instantiated or not on the current domain level.
   
.. function:: listProjectDeviceServers(string projectName, string domain = iKarabo.currentDomain)

   returns a list of all device servers in the project identified by domain and
   project name.
   
.. function:: listDevices(string domain = iKarabo.currentDomain)
   
   returns a list of devies in the specified domain

.. function:: listDevicesOnServer(string deviceServerId, string domain = iKarabo.currentDomain)
   
   returns a list of devices on the device server identified with deviceServerId
   
.. function:: listHosts(string domain = iKarabo.currentDomain)
   
   returns a list of hosts in use at this domain level
   
.. function:: listProjectHosts(string projectName, string domain = iKarabo.currentDomain)
   
   returns a list of hosts in use by the project
   
.. function:: listProjectServerGroups(string projectName, string domain = iKarabo.currentDomain)
   
   returns a list of device server groups in this project
   
.. function:: listServerDeviceGroups(string deviceServerId, string domain = iKarabo.currentDomain)
   
   returns a list of device groups on this server

.. function:: listDomainMacros(string domain = iKarabo.currentDomain)

   returns a list of macros in a domain project

.. function:: listProjectMacros(string projectName, string domain = iKarabo.currentDomain)

   returns a list of macros in this project

   
   
For all project related queries sub-projects are recursively taken into account.

The aforementioned commands may be further specified by appending the following
classifiers:

.. function::  .active
   
   only active servers or opened projects will be listed.
   
.. function:: .inactive
   
   only inactive servers or non-opened projects are returned
   
.. function:: .detailed
   
   may be appended directly or to the *.active* or *.inactive* classifiers and will lead
   to a more detailed representation being printed out on the console. Projects will also
   list the device servers they contain, device servers will give additional information,
   also on the devices.
   

   
Querying Configurations
+++++++++++++++++++++++

.. function:: listProjectSnapshots(string projectName, string domain = iKarabo.currentDomain)
   
   returns a list of configuration snapshots for this project
   
.. function:: listConfigurations(string deviceServerName, string instanceNames=None string domain = iKarabo.currentDomain)
   
   returns a list of configurations for each device instance bound to the server identified
   by *deviceServerName* or optionally further specified by *instanceNames*
   
..note::

   listConfigurations may be further specified by the *.default* and *.user(uid)* classifiers
   allowing to query only default or user configurations. The latter optionally further
   specified by an additional id.
   
.. function:: listConfigurationVersions(string instanceName, configurationName, string domain = iKarabo.currentDomain)
   
   returns a list of version for the named configuration specified by *configurationName*
   
   
Saving Configurations and Snapshots
+++++++++++++++++++++++++++++++++++

Configurations may be saved using the following commands. Saving to an existing named
configuration will create a new version of this configuration.

.. function:: saveProjectSnapshot(string projectName, [string snapshotName], string domain = iKarabo.currentDomain)
   
   saves a snapshot of the current configuration in use in this project and all the
   entities it contains as described in Section `Project & Server Snapshots`_.
   If *snapshotName* is not specified a new version of the snapshot currently, selected
   for the project is created. Setting the snapshot name to "default" will result in all
   entities contained in this project to save to their default snapshot as well.
   
.. note::

	Executing *saveProjectSnapshot* will propagate down the project hierarchy, and 
	subprojects as well as device servers will save to the snapshot they are currently
	configured to use. The only exception is if "default" is used as snapshot name. In 
	this case, as mentioned above, all entities in the hierarchy will save to the
	default snapshot.
	
.. function:: saveServerSnapshot(string projectName, string serverName, [string snapshotName], string domain = iKarabo.currentDomain)
	
	saves a snapshot of the current configuration in use in this server.
	

.. _project_loading:

Loading Configurations and Snapshots
++++++++++++++++++++++++++++++++++++

.. function:: loadInstanceConfiguration(string instanceId, [configurationName], [version], string domain = iKarabo.currentDomain)
   
   loads the configuration of the device instance specified by *instanceId* and
   returns it as as Hash object. Optionally, the configurationName may be specified as
   as string of the form "[type]/name" or "[type]/[uid]/name". Where type is either
   "default", "user" or "last_working". For user configurations the uid should further
   specify the user id. If no configurationName is given the last known working 
   configuration (i.e. "last_working") is loaded. An optional version identifier may
   further be specified.
   
.. function:: loadServerConfiguration(string projectName, string serverName, [snapshotName], [version] string domain = iKarabo.currentDomain)
   
   loads the server configuration specified by *projectName* and *serverName*, then
   returns it as as Hash object. Optionally, the snapshotName may be specified as
   as string of the form "[type]/name" or "[type]/[uid]/name". Where type is either
   "default", "user" or "last". For user snapshots the uid should further
   specify the user id. If no snapshotName is given the last used snapshot is loaded.
   An optional version identifier may further be specified.
   
.. function:: loadProjectConfiguration(string projectName, [snapshotName], [version] string domain = iKarabo.currentDomain)
   
   loads the project configuration specified by *projectName* and
   returns it as as Hash object. Additionally, macros are made available as
   instances under there name. Optionally, the snapshotName may be specified as
   as string of the form "[type]/name" or "[type]/[uid]/name". Where type is either
   "default", "user" or "last". For user snapshots the uid should further
   specify the user id. If no snapshotName is given the last used snapshot is loaded.
   An optional version identifier may further be specified. If "default" is specified
   as *snapshotName* this will propagate through the hierarchy, i.e. all member entities
   of the project will also load their "default" snapshots.
   The Hash will also contain information on the projects member entities.

.. function:: loadProjectMacros(string projectName, [snapshotName], [version] string domain = iKarabo.currentDomain)

   loads only the macros in a project specified by *projectName* and
   and makes them available as instance, . Optionally, the snapshotName may be specified as
   as string of the form "[type]/name" or "[type]/[uid]/name". Where type is either
   "default", "user" or "last". For user snapshots the uid should further
   specify the user id. If no snapshotName is given the last used snapshot is loaded.
   An optional version identifier may further be specified. If "default" is specified
   as *snapshotName* this will propagate through the hierarchy, i.e. all member entities
   of the project will also load their "default" snapshots.
   The Hash will also contain information on the projects member entities.
   
.. warning::

	Generally it is recommended to load and instantiate projects through the GUI as the
	CLI cannot display the panels configured as part of projects.
	The load operations should rather be used to retrieve configurations to be pushed
	to other instances using the methods described in `Modifying Configurations`_.
   
Modifying Configurations
++++++++++++++++++++++++

The following commands allow to modify device configurations by supplying a Hash containing
the new configuration.

.. function:: pushClassConfigurationToDomain(Hash config, string classId, string domain = iKarabo.currentDomain)
   
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change. 
    
.. function:: pushClassConfigurationToProject(Hash config, string classId, string projectName, string domain = iKarabo.currentDomain)
   
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change.
    
.. function:: pushClassConfigurationToServer(Hash config, string classId, string serverName, string domain = iKarabo.currentDomain)
   
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change. 
    
.. function:: pushServerConfigurationToDomain(Hash config, string domain = iKarabo.currentDomain)
   
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change. 
    
.. function:: pushServerConfigurationToProject(Hash config, string projectName, string domain = iKarabo.currentDomain)
   
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change.
    
.. function:: pushServerConfigurationToServer(Hash config, string serverName, string domain = iKarabo.currentDomain)
  
   pushes a configuration to all devices of the matching class. This configuration should
   already be filtered to the properties you would like to change.

.. note::

	When pushing server configurations the configuration should be assigned in three 
	top-level node elements: *server*, *classes* and *instances*. Configuration under
	*server* is pertinent to the server, configuration in *classes* to the
	classIds grouped as sub-nodes with the classId as key; and finally, configuration
	*instances* refers to the instance ids. 
	
The instance id may contain "*" wild-cards, allowing to push configurations across
subcomponent groups,

.. code-block:: Python

   configPSlit = Hash("softLimits", [-5, 5])
   configServer = Hash("instances", Hash("*/MID_*_PSLIT*", configPSlit))
   
   pushServerConfigurationToDomain(configServer)
   
is thus a valid call and would alter all devices which have an instance id matching the
wild-carded statement "\*/MID_\*_PSLIT\*" to update their softLimits property to [-5,5].


Instantiating Projects, Servers and Devices
+++++++++++++++++++++++++++++++++++++++++++

Projects, servers and devices may be instantiated from the CLI, optionally using the
configuration hashes returned by the load operations introduced in 
Section `Loading Configurations and Snapshots`_. If no configuration is given, the last
used project/server snapshot or the last known working configuration is used.

.. function:: initProject(string projectName, [config], string domain = iKarabo.currentDomain, force=False)
  
  instantiates the project specified by projectName and all its members. If a required
  device server is not instantiated on the relevant host it will be started.
  
  
.. function:: initServer(string projectName, string serverName, [config], string domain = iKarabo.currentDomain, force=False)
  
  instantiates the server specified by serverName and projectName and its device instances.
  If the required device server is not instantiated on the relevant host it will be 
  started.
  
.. function:: init(string deviceId, [config], string domain = iKarabo.currentDomain)
  
  instantiates the device specified by device id. If the required device server is not 
  instantiated on the relevant host it will be started.
  
.. warning::

	Instantiating servers or projects may conflict with already running instances and 
	their configurations. To resolve this two option exist. Setting the *force* parameter
	to true, which should be carefully used as it will force the shutdown of running devices
	and device servers and then re-instantiate them with the new configuration. Or, as the
	preferred option, acknowledging the new configurations which were pushed to already
	running device instances.
	

Acknowledging Configuration Updates
+++++++++++++++++++++++++++++++++++

A conflicting configuration may have been pushed to a running device instance, either
through the configuration push operations (Section `Modifying Configurations`_ )
or through project/server instantiation 
(Section `Instantiating Projects, Servers and Devices`_). In both case the configuration
update needs to be acknowledge before it is performed. This can be done using the 
following command.

.. function:: acknowledgeUpdatedConfig(string projectName, [string serverName], [string instanceId], [string propertyName], string domain = iKarabo.currentDomain)
   
   acknowledges the configuration update request. All parameters except project name 
   are optional and if given further specify the acknowledgement. All classification 
   parameters may also be given as lists of strings.
   
.. note::

	You may only acknowledge update requests if you have the appropriate rights to do
	so.
	
Shutting Down Project, Server and Device Instances
++++++++++++++++++++++++++++++++++++++++++++++++++

Finally, projects, device servers and device instances may shutdown. This is done with
the following commands.

.. function:: shutdownProject(string projectName, killServers=False, string domain = iKarabo.currentDomain)
   
   will shutdown all instances in the project and its sub-projects. If the killServers
   parameter is set to True, device servers will also be shut-down. The projectName may
   also be given as a list of strings.
   
   
.. function:: shutdownServer(string projectName, string serverName, killServer=False, string domain = iKarabo.currentDomain)
   
   will shutdown and instances in the device server. If the killServer
   parameter is set to True, the device server will also be shut-down. The projectName and
   serverName parameters may also be given as a list of strings.

.. function:: kill(string instanceId, string domain = iKarabo.currentDomain)
   
   shuts down the device instance specified by instance id. There is no option to kill the
   server at the same time.
   
.. todo::

	Check if this interface is feature-completed. If so, implementing a model to implement
	it would provide the required API for the GUI to use as well.


The Central Instantiation Service
+++++++++++++++++++++++++++++++++

The central instantiation service is a Karabo device that resolves
configurations provided by the database in a flat hierarchy of currently
running configurations and monitors the device's statuses, and instantiates,
or re-instantiates these as necessary, respecting run-levels while doing so.
It is the central service which has a complete overview of a given Karabo
installation. Additionally, it exposes the above interface in terms of slots
to the rest of the Karabo world.

	