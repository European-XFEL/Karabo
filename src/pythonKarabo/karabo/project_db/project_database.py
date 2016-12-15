import os
from contextlib import ContextDecorator
from time import gmtime, strftime

from eulexistdb.db import ExistDB
from eulexistdb.exceptions import ExistDBException
from lxml import etree

from karabo.common.project.api import EXISTDB_INITIAL_VERSION
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

    def get_versioning_info(self, domain, item):
        """
        Returns a dict object containing the versioning info for a given
        path.
        :param domain: domain of ``item``
        :param item: UUID of the item for which versioning information is to be
                     extracted
        :return: a dict with the following entries:
                document: the path of the document
                revisions: a list of revisions, where each entry is a dict
                with:
                    revision: the revision number
                    date: the date this revision was added
                    user: the user that added this revision
                    alias: an alias for the revision. If none is set the
                           revision number is returned


        :raises: ExistDBException if user is not authorized for this
                 transaction or the versioning query to the database failed
                 otherwise.
                 RuntimeError: if versioning information could not be extracted
                 from the query results.
        """
        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)
        query = """
        xquery version "3.0";

        let $uuid := "{uuid}"
        let $path := "{path}"

        return <versions>{{for $doc in collection($path)
                           where $doc/*/@uuid = $uuid
                           return <version revision="{{$doc/*/@revision}}"
                                           user= "{{$doc/*/@user}}"
                                           date="{{$doc/*/@date}}"
                                           alias="{{$doc/*/@alias}}" />
                           }}</versions>
        """.format(uuid=item, path=path)

        try:
            res = self.dbhandle.query(query)
            rxml = res.results[0]
        except ExistDBException as e:
            raise ProjectDBError(e)

        doc = self._make_xml_if_needed(rxml)
        revisions = [{'revision': int(c.get('revision')),
                      'user': c.get('user'),
                      'date': c.get('date'),
                      'alias': c.get('alias', c.get('revision'))}
                     for c in doc.getchildren()]
        return {'document': "{}/{}".format(path, item),
                'revisions': revisions}

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

        # Extraction the revision number as provided
        item_tree = self._make_xml_if_needed(item_xml)
        revision = int(item_tree.get('revision', EXISTDB_INITIAL_VERSION))
        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if 'date' not in item_tree.attrib:
            item_tree.attrib['date'] = strftime("%Y-%m-%d %H:%M:%S", gmtime())

        item_xml = self._make_str_if_needed(item_xml)
        path = "{}/{}/{}_{}".format(self.root, domain, uuid, revision)
        success = False
        try:
            success = self.dbhandle.load(item_xml, path)
        except ExistDBException:
            success = False

        meta = {}
        meta['versioning_info'] = self.get_versioning_info(domain, uuid)
        meta['domain'] = domain
        meta['uuid'] = uuid
        meta['revision'] = revision
        return (success, meta)

    def load_item(self, domain, item, revision):
        """
        Load an item from `domain`
        :param domain: a domain to load from
        :param item: the name of the item to load
        :param revision: revision number of item
        :return:
        """
        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)
        query = """
        xquery version "3.0";

        let $uuid := "{uuid}"
        let $rev := "{revision}"
        let $collection := "{path}"

        for $doc in collection($collection)
            where $doc//@uuid = $uuid and $doc//@revision = $rev
            return $doc
        """.format(uuid=item, revision=revision, path=path)

        try:
            res = self.dbhandle.query(query)
            return self._make_str_if_needed(res.results[0])
        except ExistDBException as e:
            raise ProjectDBError(e)
        except IndexError:
            msg = "Project object not found! (UUID: {}, revision: {})"
            raise ProjectDBError(msg.format(item, revision))

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
        let $uuids := distinct-values(collection($path)/*/@uuid)
        return <items>{{for $uuid in $uuids
            for $doc in collection($path)[*/@uuid = $uuid][1]
            {maybe_where}
            return <item uuid="{{$doc/*/@uuid}}"
                         simple_name="{{$doc/*/@simple_name}}"
                         item_type="{{$doc/*/@item_type}}" />
        }}</items>
        """
        maybe_let, maybe_where = '', ''
        if item_types is not None:
            maybe_let_tmp = "let $item_types := ('{}')"
            maybe_let = maybe_let_tmp.format("','".join(item_types))
            maybe_where = 'where $doc/*/@item_type = $item_types'

        query = query.format(maybe_let=maybe_let,
                             maybe_where=maybe_where,
                             path=path)
        try:
            res = self.dbhandle.query(query)
            return [{'uuid': r.attrib['uuid'],
                     'revisions': self._get_rev_info(domain, r.attrib['uuid']),
                     'item_type': r.attrib['item_type'],
                     'simple_name': r.attrib['simple_name']}
                    for r in res.results[0].getchildren()]
        except ExistDBException as e:
                raise ProjectDBError(e)

    def _get_rev_info(self, domain, item):
        return self.get_versioning_info(domain, item)['revisions']

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
