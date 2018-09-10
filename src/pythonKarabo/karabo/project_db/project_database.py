import os
from contextlib import ContextDecorator
from textwrap import dedent
from time import gmtime, strftime, strptime

from eulexistdb.db import ExistDB
from eulexistdb.exceptions import ExistDBException
from lxml import etree

from .util import assure_running, ProjectDBError
from .dbsettings import DbSettings

DATE_FORMAT = '%Y-%m-%d %H:%M:%S'


class ProjectDatabase(ContextDecorator):

    def __init__(self, user, password, server=None, port=None,
                 test_mode=False):
        """
        Create a project data base context for a given user
        :param user: the user, can be either admin for local db, or LDAP
        :param password: user's password. Is 'karabo' in local context
        :param server: server to connect to. If left blank the
           'KARABO_PROJECT_DB' environment variable will be used
        :param port: the port which to connect to, defaults to 8080 or the
           'KARABO_PROJECT_DB_PORT' environment variable if present
        :param test_mode: defaults to False. In this case
           db_settings.root_collection will be used as the root collection,
           otherwise, root_collection_test is used.
        :return: a ProjectDatabase context
        """
        # get our environment straightened out

        if server is None:
            server = os.getenv('KARABO_PROJECT_DB', None)

        if server is None:
            raise EnvironmentError("No environment variable KARABO_PROJECT_DB"
                                   " found, nor was a server to connect to"
                                   " given!")
        if port is None:
            port = os.getenv('KARABO_PROJECT_DB_PORT', 8080)

        self.settings = DbSettings(user, password, server, port)
        self.root = (self.settings.root_collection if not test_mode else
                     self.settings.root_collection_test)

    def __enter__(self):
        # assure there is a database running where we assume one would be
        assure_running(self.settings.server, self.settings.port)
        self.dbhandle = ExistDB(self.settings.server_url)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Clean-up action for the ProjectDatabase context. As the database
        handle doesn't carry any state nothing needs to be done here.
        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :return:
        """
        # nothing to do as dbhandle does not keep the connection open
        pass

    def domain_exists(self, domain):
        """
        Checks if a given domain exists.
        :param domain: the domain to check
        :return: True if it exists, false otherwise
        :raises: ExistDBException if user is not authorized for this query
        """
        path = "{}/{}".format(self.root, domain)
        return self.dbhandle.hasCollection(path)

    def add_domain(self, domain):
        """
        Adds a domain to the project database. A domain is a top-level
        collection located directly underneath the self.root collection. When
        created the following collections will be added to the domain:
        projects, scenes, macros, configs, device_servers, and resources.

        :param domain: the name of the domain to be created
        :return:None
        :raises: ExistDBException if user is not authorized to create this
                 collection. RuntimeError if domain creation failed otherwise
        """
        path = "{}/{}".format(self.root, domain)
        success = self.dbhandle.createCollection(path)

        if not success:
            raise RuntimeError("Failed to create domain at {}".format(path))

    @staticmethod
    def _make_xml_if_needed(xml_rep):
        """
        Returns an etree xml object from xml_rep
        :param xml_rep: the xml
        :return: a root node for the xml object
        :raises: ValueError if the object passed is not of type str or type
                 etree.ElementBase
        """
        if isinstance(xml_rep, etree._Element):
            return xml_rep
        if isinstance(xml_rep, str):
            try:
                return etree.fromstring(xml_rep)
            except etree.XMLSyntaxError:
                raise ValueError("XML syntax error encountered while parsing!")
        raise ValueError("Cannot handle type {}".format(type(xml_rep)))

    @staticmethod
    def _make_str_if_needed(xml_rep):
        """
        Returns a string representation of xml_rep
        :param xml_rep: the xml
        :return: a string representation of xml_rep
        :raises: ValueError if the object passed is not of type str or type
                 etree.ElementBase
        """
        if isinstance(xml_rep, str):
            return xml_rep
        if isinstance(xml_rep, etree._Element):
            xml_bytes = etree.tostring(xml_rep, pretty_print=True,
                                       encoding="unicode",
                                       xml_declaration=False)
            return xml_bytes

        raise ValueError("Cannot handle type {}".format(type(xml_rep)))

    def _check_for_modification(self, domain, uuid, old_date):
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param domain: the domain under which this item exists
        :param uuid: the item's uuid
        :param old_date: the item's last modified time stamp
        :return A tuple stating whether the item was modified inbetween and a
                string describing the reason
        """
        # Check whether the object got modified inbetween
        path = "{}/{}".format(self.root, domain)
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

    def save_item(self, domain, uuid, item_xml, overwrite=False):
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


        :raises: ExistDBException if user is not authorized for this query.
            RuntimeError if item saving failed otherwise.
            AttributeError if a non-supported type is passed
        """

        # create domain if necessary
        if not self.domain_exists(domain):
            self.add_domain(domain)

        # Extract some information
        try:
            # NOTE: The client might send us garbage
            item_tree = self._make_xml_if_needed(item_xml)
        except ValueError:
            msg = 'XML parse error for item "{}"'.format(uuid)
            raise ProjectDBError(msg)

        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if not item_tree.attrib.get('date'):
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())
        else:
            modified, reason = self._check_for_modification(
                domain, uuid, item_tree.attrib['date'])
            if modified:
                raise ProjectDBError(reason)
            # Update time stamp
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())

        # XXX: Add a revision/alias to keep old code from blowing up
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        item_xml = self._make_str_if_needed(item_tree)
        # XXX: Add a '_0' suffix to keep old code from wetting its pants
        path = "{}/{}/{}_0".format(self.root, domain, uuid)

        try:
            if self.dbhandle.hasDocument(path) and not overwrite:
                raise ExistDBException("Versioning conflict. Document exists!")
            # NOTE: The underlying HTTP code needs bytes here...
            success = self.dbhandle.load(item_xml.encode('utf8'), path)
        except ExistDBException as e:
            raise ProjectDBError(e)
        if not success:
            raise ProjectDBError("Saving item failed!")

        meta = {}
        meta['domain'] = domain
        meta['uuid'] = uuid
        meta['date'] = item_tree.attrib['date']
        return meta

    def load_item(self, domain, items):
        """
        Load an item or items from `domain`
        :param domain: a domain to load from
        :param item: the name of the item(s) to load
        :return:
        """
        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)

        assert isinstance(items, (list, tuple))

        results = []
        n_items = len(items)
        # we re-chunk the request for querying as everything ends up in a
        # single get call to the restful API. Through trial 50 seems a
        # reasonable trade-off between size of query and return value size.
        req_cnk_size = 50

        for i in range(0, n_items, req_cnk_size):
            max_idx = min(i+req_cnk_size, n_items)
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

            """.format(uuids='("{}")'.format(uuids), path=path)

            try:
                res = self.dbhandle.query(query, how_many=req_cnk_size)
                for r in res.results:
                    results.append({'uuid': r.get('uuid'),
                                    'xml': self._make_str_if_needed(r)})
            except ExistDBException as e:
                raise ProjectDBError(e)

        return results

    def list_items(self, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)
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

    def list_domains(self):
        """
        List top level domains in database
        :return:
        """

        query = """
                xquery version "3.0";
                <collections>{{
                for $c in xmldb:get-child-collections("{}")
                return <item>{{$c}}</item>}}
                </collections>
                """.format(self.root)

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
        origin_base = '/'.join(domain.split('/')[:-1])+"/"
        base = '{}{}/'.format('/'.join(domain.split('/')[:-2]),
                              self.settings.root_collection_backup)
        source = domain.split('/')[-1]
        bck = "{}_{}_backup".format(source, tstamp)
        tmp = "tmp_{}".format(tstamp)

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

        print("Creating backup of domain {} at {}{}".format(domain, base, bck))
        try:
            res = self.dbhandle.query(query)
        except ExistDBException as e:
            raise("Failed creating backup: {}".format(e))

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
            raise("Failed sanitizing: {}".format(e))

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

    def update_attributes(self, items):
        """ Update attribute for the given ``items``

        :param items: list of Hashes containing information on which items
                      to update. Each list entry should be a Hash containing

                      - domain: domain the item resides at
                      - uuid: the uuid of the item
                      - item_type: indicate type of item which attribute should
                                   be changed
                      - attr_name: name of attribute which should be changed
                      - attr_value: value of attribute which should be changed

        :return: a list of dicts where each entry has keys: domain, uuid,
                 item_type, attr_name, attr_value
        """
        res_items = []
        for it in items:
            domain = it.get('domain')
            item_type = it.get('item_type')
            uuid = it.get('uuid')
            attr_name = it.get('attr_name')
            attr_value = it.get('attr_value')
            # XXX: Add a '_0' suffix to keep old code from wetting its pants
            path = "{}/{}/{}_0".format(self.root, domain, uuid)
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

            try:
                res = self.dbhandle.query(query)
                root = res.results[0]
                res_items.append({'domain': domain,
                                  'uuid': root.attrib['uuid'],
                                  'item_type': root.attrib['item_type'],
                                  'attr_name': root.attrib['attr_name'],
                                  'attr_value': root.attrib['attr_value']})
            except ExistDBException as e:
                raise("Failed updating attribute: {}".format(e))
        return res_items
