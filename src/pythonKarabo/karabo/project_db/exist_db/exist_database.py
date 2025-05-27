# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# flake8: noqa: E501
# this file contains xquery code
import os
from subprocess import check_call
from textwrap import dedent
from time import gmtime, sleep, strftime, strptime

from pyexistdb.db import ExistDB
from pyexistdb.exceptions import ExistDBException
from urllib3.exceptions import HTTPError

from karabo.project_db.util import ProjectDBError

from ..bases import DatabaseBase
from ..const import DATE_FORMAT
from ..util import make_str_if_needed, make_xml_if_needed
from .dbsettings import DbSettings

# The default password of the docker image:
# https://git.xfel.eu/ITDM/docker_existdb
TESTDB_ADMIN_PASSWORD = "change_me_please"


def get_db_credentials(is_test):
    # in production the KARABO_PROJECT_DB_USER and KARABO_PROJECT_DB_PASSWORD
    # should be set to the username and password of the configDB.
    # Alternatively, the username and password will be defaulted to 'karabo'
    default_user = 'admin' if is_test else 'karabo'
    default_pwd = TESTDB_ADMIN_PASSWORD if is_test else 'karabo'
    user = os.getenv('KARABO_PROJECT_DB_USER', default_user)
    password = os.getenv('KARABO_PROJECT_DB_PASSWORD', default_pwd)
    return user, password


class ExistDatabase(DatabaseBase):

    root = None

    def path(self, domain: str, uuid: str):
        # XXX: Add a '_0' suffix to keep old code from wetting its pants
        return f"{self.root}/{domain}/{uuid}_0"

    def __init__(self, user, password, server=None, port=None,
                 test_mode=False, init_db=False):
        """
        Create a project data base context for a given user
        :param user: the user, can be either admin for local db, or set on the
                     remote projectDB
        :param password: user's password. Is 'karabo' in local context
        :param server: server to connect to. If left blank the
           'KARABO_PROJECT_DB' environment variable will be used
        :param port: the port which to connect to, defaults to 8080 or the
           'KARABO_PROJECT_DB_PORT' environment variable if present
        :param test_mode: defaults to False. In this case
           db_settings.root_collection will be used as the root collection,
           otherwise, root_collection_test is used.
           If test_mode is True, the server and port options will be evaluated
           from the KARABO_TEST_PROJECT_DB and KARABO_TEST_PROJECT_DB_PORT
           variables respectively.
        :param init_db: defaults to False. If True, the default collections
           will be added if missing on context entry.
        :return: a ProjectDatabase context
        """
        super().__init__()
        if test_mode:
            server = os.getenv('KARABO_TEST_PROJECT_DB', 'localhost')
            port = os.getenv('KARABO_TEST_PROJECT_DB_PORT', 8181)

        # get our environment straightened out
        if server is None:
            server = os.getenv('KARABO_PROJECT_DB', None)

        if server is None:
            raise OSError("No environment variable KARABO_PROJECT_DB"
                          " found, nor was a server to connect to"
                          " given!")
        if port is None:
            port = os.getenv('KARABO_PROJECT_DB_PORT', 8181)

        port = int(port)
        self.dbhandle = None
        self.settings = DbSettings(user, password, server, port, init_db)
        self.root = (self.settings.root_collection if not test_mode else
                     self.settings.root_collection_test)

    async def __aenter__(self):
        self.dbhandle = assure_running(self.settings)
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        return await super().__aexit__(exc_type, exc_value, traceback)

    async def _check_for_modification(self, domain, uuid, old_date):
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param domain: the domain under which this item exists
        :param uuid: the item's uuid
        :param old_date: the item's last modified time stamp
        :return A tuple stating whether the item was modified inbetween and a
                string describing the reason
        """
        # Check whether the object got modified inbetween
        path = f"{self.root}/{domain}"
        query = """
            xquery version "3.0";
            let $path := "{path}"
            let $uuid := "{uuid}"

            for $doc in collection($path)/xml[@uuid = $uuid]
            let $date := $doc/@date
            return <item date="{{$date}}"/>
                """.format(path=path, uuid=uuid)
        modified = False
        reason = ""
        try:
            res = self.dbhandle.query(query)
            if res.results:
                current_date = res.results[0].get('date')
            else:
                # Item not yet existing
                return modified, reason
        except ExistDBException as e:
            raise ProjectDBError(e)

        current_modified = strptime(current_date, DATE_FORMAT)
        last_modified = strptime(old_date, DATE_FORMAT)
        if last_modified < current_modified:
            # Item got saved inbetween
            modified = True
            reason = "Versioning conflict! Document modified inbetween."
        return modified, reason

    async def load_item(self, domain, items):
        """
        Load an item or items from `domain`
        :param domain: a domain to load from
        :param item: the name of the item(s) to load
        :return:
        """
        # path to where the possible entries are located
        path = f"{self.root}/{domain}"

        assert isinstance(items, (list, tuple))

        results = []
        n_items = len(items)
        # we re-chunk the request for querying as everything ends up in a
        # single get call to the restful API. Through trial 50 seems a
        # reasonable trade-off between size of query and return value size.
        req_cnk_size = 50

        for i in range(0, n_items, req_cnk_size):
            max_idx = min(i + req_cnk_size, n_items)
            uuids = '","'.join(items[i:max_idx])
            query = """
            xquery version "3.0";

            let $uuids := {uuids}
            let $path := "{path}"

            for $uuid in $uuids
            let $sorted-docs :=
                for $doc in collection($path)/xml[@uuid = $uuid]
                order by $doc/@date
                return $doc
            return $sorted-docs[last()]

            """.format(uuids=f'("{uuids}")', path=path)

            try:
                res = self.dbhandle.query(query, how_many=req_cnk_size)
                for r in res.results:
                    results.append({'uuid': r.get('uuid'),
                                    'xml': make_str_if_needed(r)})
            except ExistDBException as e:
                raise ProjectDBError(e)

        return results

    async def list_items(self, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        # path to where the possible entries are located
        path = f"{self.root}/{domain}"
        query = """
        xquery version "3.0";
        declare namespace functx = "http://www.functx.com";
        declare function functx:if-absent(
            $arg as item()* , $value as item()*)  as item()*
        {{ if (exists($arg)) then $arg else $value }};

        let $path := "{path}"
        {maybe_let}
        return <items>{{
        for $doc in collection($path)/xml{maybe_where}
        let $uuid := $doc/@uuid
        let $simple_name := $doc/@simple_name
        let $item_type := $doc/@item_type
        let $is_trashed := functx:if-absent($doc/@is_trashed, 'false')
        let $user := functx:if-absent($doc/@user, '')
        let $date := functx:if-absent($doc/@date, '')
        let $description := functx:if-absent($doc/@description, '')
        group by $uuid, $simple_name, $item_type, $is_trashed, $user, $date,
        $description
        return <item uuid="{{$uuid}}"
                simple_name="{{$simple_name}}"
                item_type="{{$item_type}}"
                is_trashed="{{$is_trashed}}"
                user="{{$user}}"
                date="{{$date}}"
                description="{{$description}}" />
        }}</items>
        """
        maybe_let, maybe_where = '', ''
        if item_types is not None:
            maybe_let_tmp = "let $item_types := ('{}')"
            maybe_let = maybe_let_tmp.format("','".join(item_types))
            maybe_where = '[@item_type=$item_types]'

        query = query.format(maybe_let=maybe_let,
                             maybe_where=maybe_where,
                             path=path)
        try:
            res = self.dbhandle.query(query)
            return [{'uuid': r.attrib['uuid'],
                     'item_type': r.attrib['item_type'],
                     'simple_name': r.attrib['simple_name'],
                     'is_trashed': r.attrib['is_trashed'],
                     'user': r.attrib['user'],
                     'date': r.attrib['date'],
                     'description': r.attrib['description']}
                    for r in res.results[0].getchildren()]
        except ExistDBException as e:
            raise ProjectDBError(e)

    async def list_named_items(self, domain, item_type, simple_name):
        """
        List items in domain which match item_type and simple_name

        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: iterable of names to match
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        # path to where the possible entries are located
        path = f"{self.root}/{domain}"
        query = """
        xquery version "3.0";
        declare namespace functx = "http://www.functx.com";
        declare function functx:if-absent(
            $arg as item()* , $value as item()*)  as item()*
        {{ if (exists($arg)) then $arg else $value }};

        let $path := "{path}"
        return <items>{{
        for $doc in collection($path)/xml{where}
        let $uuid := $doc/@uuid
        let $simple_name := $doc/@simple_name
        let $item_type := $doc/@item_type
        let $is_trashed := functx:if-absent($doc/@is_trashed, 'false')
        let $user := functx:if-absent($doc/@user, '')
        let $date := functx:if-absent($doc/@date, '')
        let $description := functx:if-absent($doc/@description, '')
        group by $uuid, $simple_name, $item_type, $is_trashed, $user, $date,
        $description
        return <item uuid="{{$uuid}}"
                simple_name="{{$simple_name}}"
                item_type="{{$item_type}}"
                is_trashed="{{$is_trashed}}"
                user="{{$user}}"
                date="{{$date}}"
                description="{{$description}}" />
        }}</items>
        """
        where = "[@item_type=('{}') and @simple_name=('{}')]".format(
            item_type, simple_name)

        query = query.format(where=where,
                             path=path)
        try:
            res = self.dbhandle.query(query)
            return [{'uuid': r.attrib['uuid'],
                     'item_type': r.attrib['item_type'],
                     'simple_name': r.attrib['simple_name'],
                     'is_trashed': r.attrib['is_trashed'],
                     'user': r.attrib['user'],
                     'date': r.attrib['date'],
                     'description': r.attrib['description']}
                    for r in res.results[0].getchildren()]
        except ExistDBException as e:
            raise ProjectDBError(e)

    async def list_domains(self):
        """
        List top level domains in database
        :return:
        """
        query = LIST_DOMAINS_QUERY.format(self.root)
        try:
            res = self.dbhandle.query(query)
            return [r.text for r in res.results[0]]
        except ExistDBException:
            return []

    def sanitize_database(self, domain):
        """ This function (attempts to) sanitize all items in a domain

        NOTE: to run it
        from karabo.project_db.project_database import ProjectDatabase
        db = ProjectDatabase("karabo", "karabo", server="exflst105")
        with db as db:
            r = db.sanitize_database("/db/krb_config/CAS_INTERNAL")

        It inserts the minimally expected attributes if missing! A backup of
        the collection is created prior to the operation!
        """

        domain = domain.rstrip('/')
        tstamp = strftime("%Y-%m-%d_%H%M%S", gmtime())
        origin_base = '/'.join(domain.split('/')[:-1]) + "/"
        base = '{}{}/'.format('/'.join(domain.split('/')[:-2]),
                              self.settings.root_collection_backup)
        source = domain.split('/')[-1]
        bck = f"{source}_{tstamp}_backup"
        tmp = f"tmp_{tstamp}"

        query = """
            xquery version "3.0";
            import module namespace xmldb="http://exist-db.org/xquery/xmldb";

            let $origin_base := "{origin_base}"
            let $base := "{base}"
            let $col1 := "{source}"
            let $tmp := "{tmp}"
            let $col2 := "{dest}"

            let $res := xmldb:create-collection($base, $tmp)
            let $cpy_res := xmldb:copy(concat($origin_base,$col1), concat($base,$tmp))
            let $rnm_res := xmldb:rename(concat($base,$tmp,"/",$col1), $col2)
            let $mv_res := xmldb:move(concat($base,$tmp,"/",$col2), $base)
            return xmldb:remove(concat($base,$tmp))

        """.format(origin_base=origin_base, base=base, source=source, tmp=tmp,
                   dest=bck)

        print(f"Creating backup of domain {domain} at {base}{bck}")
        try:
            res = self.dbhandle.query(query)
        except ExistDBException as e:
            raise ProjectDBError(f"Failed creating backup: {e}")

        print("Sucessfully created backup, now proceeding with sanitizing")

        query = """
            xquery version "3.0";

            let $col := "{domain}"

            (: This query updates root elements missing aliases :)
            let $root_updates := for $doc in collection($col)/xml[not(@alias)]
            let $update := update insert attribute alias {{'default'}} into $doc
            return $doc

            (: This query updates project elements missing revisions :)
            let $docs := for $doc in collection($col)/xml[@item_type='project']
            where exists($doc/*/*/KRB_Item) and (not(exists($doc/*/*/KRB_Item/revision)))
            return $doc

            let $project_updates := for $doc in $docs
            let $update := update insert <revision KRB_Type="INT32">0</revision> into $doc/*/*/KRB_Item
            return $doc


            (: This query updates device server elements missing revisions :)
            let $device_server_updates := for $doc in collection($col)/xml[@item_type='device_server']/device_server/device_instance[not(@revision)]
            let $update := update insert attribute revision {{'0'}} into $doc
            return $doc


            (: These queries updates device config elements missing revisions :)
            let $config_updates_1 := for $doc in collection($col)/xml[@item_type='device_instance']/device_instance/device_config[not(@revision)]
            let $update := update insert attribute revision {{'0'}} into $doc
            return $doc

            let $config_updates_2 := for $doc in collection($col)/xml[@item_type='device_instance'][not(@active_rev)]
            let $update := update insert attribute active_rev {{'0'}} into $doc
            return $doc

            return <updates>
                <root_updates>{{count($root_updates)}}</root_updates>
                <project_updates>{{count($project_updates)}}</project_updates>
                <device_server_updates>{{count($device_server_updates)}}</device_server_updates>
                <config_updates_1>{{count($config_updates_1)}}</config_updates_1>
                <config_updates_2>{{count($config_updates_2)}}</config_updates_2>
                </updates>

            """.format(domain=domain)

        try:
            res = self.dbhandle.query(query).results[0]
        except ExistDBException as e:
            raise ProjectDBError(f"Failed sanitizing: {e}")

        msg = dedent("""
            Sanitized database:
            -------------------
            Root elements updated:                   {root_updates}
            Project elements updated:                {project_updates}
            Device server elements updated:          {device_server_updates}
            Configuration elements updated (root):   {config_updates_1}
            Configuration elements updated (child):  {config_updates_2}
        """.format(root_updates=res[0].text,
                   project_updates=res[1].text,
                   device_server_updates=res[2].text,
                   config_updates_1=res[3].text,
                   config_updates_2=res[4].text))
        print(msg)

    async def update_trashed(self, **info):
        """ Update istrashed for the project"""
        domain = info.get('domain')
        item_type = info.get('item_type')
        assert item_type == "project"
        uuid = info.get('uuid')
        attr_value = info.get('value')
        attr_name = "is_trashed"
        # ExistDB needs to save as string
        attr_value = str(attr_value).lower()

        path = self.path(domain, uuid)
        query = """
            xquery version "3.0";

            let $doc := doc("{path}")/xml[@item_type="{item_type}"]
            let $cond :=
                if (exists($doc/@{attr_name}))
                then update value $doc/@{attr_name} with "{attr_value}"
                else update insert attribute {attr_name} {{"{attr_value}"}}
                        into $doc

            return <doc uuid="{{$doc/@uuid}}"
                    item_type="{{$doc/@item_type}}"
                    attr_name="{attr_name}"
                    attr_value="{{$doc/@{attr_name}}}"/>
                """.format(path=path, item_type=item_type,
                            attr_name=attr_name, attr_value=attr_value)

        self.dbhandle.query(query)

    async def get_configurations_from_device_name(self, domain, instance_id):
        """
        Returns a list of configurations for a given device
        :param domain: DB domain
        :param instance_id: instance id of the device
        :return: a list of dicts:
            [{"configid": uuid of the configuration,
              "instanceid": device instance uuid in the DB},
              ...
            ]
        """
        query = """
        xquery version "3.0";
        let $path := "{path}"
        let $iid := "{instance_id}"
        return <items>{{
        for $doc in collection($path)/xml//device_instance[@instance_id=$iid]
        let $active := $doc/@active_rev
        let $configid := $doc/device_config[@revision=$active]/@uuid
        let $instanceid := $doc/../@uuid
        return <item configid="{{$configid}}" instanceid="{{$instanceid}}"/>
        }}</items>
        """
        path = f"{self.root}/{domain}"
        query = query.format(path=path, instance_id=instance_id)

        res = self.dbhandle.query(query)
        return [{"configid": r.attrib["configid"],
                 "instanceid": r.attrib["instanceid"]} for r in
                res.results[0].getchildren()]

    async def get_configurations_from_device_name_part(self, domain, device_id_part):
        """
        Returns a list of configurations for a given device
        :param domain: DB domain
        :param device_id_part: part of device name; search is case-insensitive.
        :return: a list of dicts:
            [{"config_id": uuid of the configuration,
              "device_uuid": device instance uuid in the DB,
              "device_id": device instance id},
              ...
            ]
        """
        query = """
        xquery version "3.0";
        let $path := "{path}"
        let $iid := "{instance_id}"
        return <items>{{
        for $doc in collection($path)/xml//device_instance
             [contains(lower-case(@instance_id),lower-case($iid))]
        let $active := $doc/@active_rev
        let $config_id := $doc/device_config[@revision=$active]/@uuid
        let $device_uuid := $doc/../@uuid
        let $device_id := $doc/@instance_id
        return <item config_id="{{$config_id}}"
                device_uuid="{{$device_uuid}}"
                device_id="{{$device_id}}"/>
        }}</items>
        """
        path = f"{self.root}/{domain}"
        query = query.format(path=path, instance_id=device_id_part)

        res = self.dbhandle.query(query)

        return [{"config_id": r.attrib["config_id"],
                 "device_uuid": r.attrib["device_uuid"],
                 "device_id": r.attrib["device_id"]} for r in
                res.results[0].getchildren()]

    async def get_projects_from_device(self, domain, uuid):
        """
        Returns the projects which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """
        query = """
        xquery version "3.0";
        let $path := "{path}"
        let $iid := "{uuid}"
        return <items>{{
        for $doc in collection($path)/xml//device_instance[@uuid=$iid]
        let $serverid := $doc/../../@uuid
        return <item serverid="{{$serverid}}"/>
        }}</items>
        """

        path = f"{self.root}/{domain}"
        query = query.format(path=path, uuid=uuid)
        res = self.dbhandle.query(query)

        servers = [{"serverid": r.attrib["serverid"]} for r in
                   res.results[0].getchildren()]

        query = """
        xquery version "3.0";
        let $path := "{path}"
        let $iid := "{instance}"
        return <items>{{
        for $doc in collection($path)/xml//servers[KRB_Item/uuid=$iid]
        let $projectname := $doc/../../../@simple_name
        return <item projectname="{{$projectname}}"/>
        }}</items>
        """
        projects = set()
        for server in servers:
            queryf = query.format(path=path, instance=server["serverid"])
            res = self.dbhandle.query(queryf)
            projects |= {r.attrib["projectname"] for r in
                         res.results[0].getchildren()}
        return projects

    async def get_projects_data_from_device(self, domain, uuid):
        """
        Returns the projects which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a list containing project names, uuids and last modification
                 data.
        """
        query = """
        xquery version "3.0";
        let $path := "{path}"
        let $iid := "{uuid}"
        return <items>{{
        for $doc in collection($path)/xml//device_instance[@uuid=$iid]
        let $serverid := $doc/../../@uuid
        return <item serverid="{{$serverid}}"/>
        }}</items>
        """

        path = f"{self.root}/{domain}"
        query = query.format(path=path, uuid=uuid)
        res = self.dbhandle.query(query)

        servers = [{"serverid": r.attrib["serverid"]} for r in
                   res.results[0].getchildren()]

        query = """
        xquery version "3.0";
        declare namespace functx = "http://www.functx.com";
        declare function functx:if-absent(
            $arg as item()* , $value as item()*)  as item()*
        {{ if (exists($arg)) then $arg else $value }};

        let $path := "{path}"
        let $iid := "{instance}"
        return <items>{{
        for $doc in collection($path)/xml//servers[KRB_Item/uuid=$iid]
        let $projectname := $doc/../../../@simple_name
        let $date := functx:if-absent($doc/../../../@date, '')
        let $uuid := $doc/../../../@uuid
        return <item projectname="{{$projectname}}"
                     date="{{$date}}"
                     uuid="{{$uuid}}" />
        }}</items>
        """
        projects = []
        for server in servers:
            queryf = query.format(path=path, instance=server["serverid"])
            res = self.dbhandle.query(queryf)
            for r in res.results[0].getchildren():
                if r.attrib["projectname"] and r.attrib["uuid"]:
                    # Project data is valid; cases of projects with empty
                    # names have been found during tests.
                    projects.append({
                        "projectname": r.attrib["projectname"],
                        "date": r.attrib["date"],
                        "uuid": r.attrib["uuid"]
                    })
        return projects

    def domain_exists(self, domain: str):
        """
        Checks if a given domain exists.
        :param domain: the domain to check
        :return: True if it exists, false otherwise
        """
        path = f"{self.root}/{domain}"
        return self.dbhandle.hasCollection(path)

    def add_domain(self, domain: str):
        """
        Adds a domain to the project database. A domain is a top-level
        collection located directly underneath the self.root collection. When
        created the following collections will be added to the domain:
        projects, scenes, macros, configs, device_servers, and resources.

        :param domain: the name of the domain to be created
        :return:None
        :raises: ProjectDBError on Handle failure,
                 or RuntimeError if domain creation failed otherwise
        """
        path = f"{self.root}/{domain}"
        success = self.dbhandle.createCollection(path)

        if not success:
            raise RuntimeError(f"Failed to create domain at {path}")

    async def save_item(self, domain, uuid, item_xml, overwrite=False):
        """
        Saves a item xml file into the domain. It will
        create a new entry if the item does not exist yet, or create a new
        version of the item if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        item file is returned.

        The root node of the xml should contain a `item_type` entry identifying
        the type of the item as one of the following:

        'projects', 'scenes', 'macros', 'device_configs', 'device_servers'

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this item is to be stored
        :param uuid: the item's uuid
        :param item_xml: the xml containing the item information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the item.


        :raises: ProjectDBError on Handle Failure.
            RuntimeError if item saving failed otherwise.
            AttributeError if a non-supported type is passed
        """

        # create domain if necessary
        if not self.domain_exists(domain):
            self.add_domain(domain)

        # Extract some information
        try:
            # NOTE: The client might send us garbage
            item_tree = make_xml_if_needed(item_xml)
        except ValueError:
            msg = f'XML parse error for item "{uuid}"'
            raise ProjectDBError(msg)

        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if not item_tree.attrib.get('date'):
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())
        else:
            modified, reason = await self._check_for_modification(
                domain, uuid, item_tree.attrib['date'])
            if modified:
                message = "The <b>{}</b> item <b>{}</b> could not be saved: " \
                          "{}".format(item_tree.attrib.get('item_type', ''),
                                      item_tree.attrib.get('simple_name', ''),
                                      reason)
                raise ProjectDBError(message)
            # Update time stamp
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())

        # XXX: Add a revision/alias to keep old code from blowing up
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        item_xml = make_str_if_needed(item_tree)
        path = self.path(domain, uuid)

        if self.dbhandle.hasDocument(path) and not overwrite:
            raise ProjectDBError("Versioning conflict. Document exists!")
        # NOTE: The underlying HTTP code needs bytes here...
        success = self.dbhandle.load(item_xml.encode('utf8'), path)
        if not success:
            raise ProjectDBError("Saving item failed!")

        meta = {}
        meta['domain'] = domain
        meta['uuid'] = uuid
        meta['date'] = item_tree.attrib['date']
        return meta

    async def get_projects_with_device(
            self, domain: str, device_id_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects that contain
        active configurations for a given device.

        :param domain: DB domain
        :param device_id_part: part of name of devices for which project data
                               must be returned.
        :return: a list of dicts:
            [{"projectname": name of project,
              "date": last modification timestamp for the project,
              "uuid": uuid of projecti
              "devices": list of ids of prj devices with the given part}, ...]
        """
        configs = await self.get_configurations_from_device_name_part(
            domain, device_id_part)

        projects = []
        for config in configs:
            device_uuid = config["device_uuid"]
            device_id = config["device_id"]
            for prj in await self.get_projects_data_from_device(domain,
                                                                device_uuid):
                prj_in_list = next((p for p in projects
                                    if p["uuid"] == prj["uuid"]), None)
                if prj_in_list:
                    prj_in_list["items"].append(device_id)
                else:
                    prj["items"] = [device_id]
                    projects.append(prj)

        return projects

    async def get_projects_with_conf(self, domain, device_id):
        """
        Returns a dict with projects and active configurations from a device
        name.

        :param domain: DB domain
        :param device_id: the device to return the information for.
        :return: a dict:
            {"project name": configuration uuid,
             ...}
        """
        configs = await self.get_configurations_from_device_name(
            domain, device_id)
        projects = dict()
        for config in configs:
            instance_id = config["instanceid"]
            for project in await self.get_projects_from_device(
                    domain, instance_id):
                projects[project] = config["configid"]
        return projects


LIST_DOMAINS_QUERY = """
    xquery version "3.0";
    <collections>{{
    for $c in xmldb:get-child-collections("{}")
    return <item>{{$c}}</item>}}
    </collections>
    """


def assure_running(db_settings):
    """
    Assures an instance of existDB is running on your **local** host if the
    KARABO_PROJECT_DB environment variable points to localhost or has not been
    set. Otherwise tries to contact the remote database to assure that the data
    base is reachable
    :param db_settings, a DbSettings object
    :return: None
    """
    karabo_install = os.getenv('KARABO')

    if db_settings.server == 'localhost':
        # we execute the start script for the database.
        # If a database is already started, the script will not start a second
        script_path = os.path.join(karabo_install, 'bin',
                                   'karabo-startprojectdb')
        check_call([script_path])
        # wait until the database is actually up
        max_timeout = 60
        wait_between = 5
        count = 0
        while True:
            try:
                return verify_db(db_settings)
            except (TimeoutError, HTTPError, ExistDBException) as last_ex:
                if count > max_timeout//wait_between:
                    raise TimeoutError("Starting project database timed"
                                       " out! Last exception: {}"
                                       .format(last_ex))
            sleep(wait_between)
            count += 1
    else:
        try:
            return verify_db(db_settings)
        except ExistDBException as e:
            raise ProjectDBError("Could not contact the database server"
                                 " at {}: {}".format(db_settings.server, e))


def init_db(db_settings, dbhandle):

    def init_collection(coll_name):
        if not dbhandle.hasCollection(coll_name):
            dbhandle.createCollection(coll_name)

    # test root
    init_collection(db_settings.root_collection_test)
    # root
    init_collection(db_settings.root_collection)
    try:
        query = LIST_DOMAINS_QUERY.format(db_settings.root_collection)
        r = dbhandle.query(query)
        assert len(r.results[0]) != 0
    except (ExistDBException, AssertionError):
        # LOCAL domain
        init_collection(f"{db_settings.root_collection}/LOCAL")


def verify_db(db_settings):
    dbhandle = ExistDB(db_settings.server_url)
    if db_settings.init_db:
        init_db(db_settings, dbhandle)
    if not dbhandle.hasCollection(db_settings.root_collection):
        raise ProjectDBError("An eXistDB instance without karabo "
                             "collections was found running on {}."
                             .format(db_settings.server))
    if dbhandle.hasCollection('/system'):
        return dbhandle
