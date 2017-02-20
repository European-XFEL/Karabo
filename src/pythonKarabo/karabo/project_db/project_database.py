import os
from contextlib import ContextDecorator
from time import gmtime, strftime

from eulexistdb.db import ExistDB
from eulexistdb.exceptions import ExistDBException
from lxml import etree

from .util import assure_running, ProjectDBError
from .dbsettings import DbSettings


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
        :raises: AttributeError if the object passed is not of type str or type
                 etree.ElementBase
        """
        if isinstance(xml_rep, etree._Element):
            return xml_rep
        if isinstance(xml_rep, str):
            return etree.fromstring(xml_rep)
        raise AttributeError("Cannot handle type {}".format(type(xml_rep)))

    @staticmethod
    def _make_str_if_needed(xml_rep):
        """
        Returns a string representation of xml_rep
        :param xml_rep: the xml
        :return: a string representation of xml_rep
        :raises: AttributeError if the object passed is not of type str or type
                 etree.ElementBase
        """
        if isinstance(xml_rep, str):
            return xml_rep
        if isinstance(xml_rep, etree._Element):
            xml_bytes = etree.tostring(xml_rep, pretty_print=True,
                                       encoding="unicode",
                                       xml_declaration=False)
            return xml_bytes

        raise AttributeError("Cannot handle type {}".format(type(xml_rep)))

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
        item_tree = self._make_xml_if_needed(item_xml)
        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if 'date' not in item_tree.attrib:
            item_tree.attrib['date'] = strftime("%Y-%m-%d %H:%M:%S", gmtime())
        # XXX: Add a revision/alias to keep old code from blowing up
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        item_xml = self._make_str_if_needed(item_tree)
        path = "{}/{}/{}".format(self.root, domain, uuid)

        try:
            if self.dbhandle.hasDocument(path) and not overwrite:
                raise ExistDBException("Versioning conflict. Document exists!")
            success = self.dbhandle.load(item_xml, path)
        except ExistDBException as e:
            raise ProjectDBError(e)
        if not success:
            raise ProjectDBError("Saving item failed!")

        meta = {}
        meta['domain'] = domain
        meta['uuid'] = uuid
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
        let $path := "{path}"
        {maybe_let}
        return <items>{{
        for $doc in collection($path)/xml{maybe_where}
        let $uuid := $doc/@uuid
        let $simple_name := $doc/@simple_name
        let $item_type := $doc/@item_type
        group by $uuid, $simple_name, $item_type
        return <item uuid="{{$uuid}}"
                     simple_name="{{$simple_name}}"
                     item_type="{{$item_type}}" />
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
                     'simple_name': r.attrib['simple_name']}
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
                xmldb:get-child-collections("{}")
                """.format(self.root)

        try:
            res = self.dbhandle.query(query)
            return [r.text for r in res.results]
        except ExistDBException:
            return []
