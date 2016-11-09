import os
import time
from contextlib import ContextDecorator

from eulexistdb import db
from eulexistdb.exceptions import ExistDBException
from lxml import etree

from .util import assure_running
from .dbsettings import DbSettings


class ProjectDatabase(ContextDecorator):

    vers_namespace = "http://exist-db.org/versioning"

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
        self.root = self.settings.root_collection if not test_mode else\
            self.settings.root_collection_test

    def __enter__(self):
            # assure there is a database running where we assume one would be
            assure_running(self.settings.server, self.settings.port)
            self.dbhandle = db.ExistDB(self.settings.server_url)
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

    def get_versioning_info_item(self, domain, item):
        """
        Returns the versioning info for an item identified by its uuid
        :param domain: domain item is located at
        :param item: to retrieve versioning info for
        :return: see :ref:`get_versioning_info`
        """

        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)

        query = """
                xquery version "3.0";
                for $c at $i in collection("{path}?select=*")
                where $c/*/@uuid = "{uuid}"
                return document-uri($c)
                """.format(path=path, uuid=item)
        try:
            res = self.dbhandle.query(query)
            return self.get_versioning_info(res.results[0].text)
        except ExistDBException:
            return {}

    def get_versioning_info(self, path):
        """
        Returns a dict object containing the versioning info for a given
        path.
        :param path: path to item for which versioning information is to be
                     extracted
        :return: a dict with the following entries:
                document: the path of the document
                revisions: a list of revisions, where each entry is a dict
                with:
                    revision: the revision number
                    date: the date this revision was added
                    user: the user that added this revision
        :raises: ExistDBException if user is not authorized for this
                 transaction or the versioning query to the database failed
                 otherwise.
                 RuntimeError: if versioning information could not be extracted
                 from the query results.
        """

        query = ('import module namespace v="{}";'
                 'v:history(doc("{}"))'.format(self.vers_namespace, path))
        result = self.dbhandle.query(query)
        if result.count != 1 or result.hits != 1:
            raise RuntimeError("Expected exactly one result when querying"
                               "version information for {}, but got {} hits"
                               "and {} count"
                               .format(path, result.hits, result.count))

        # process result
        rdict = {}
        rxml = result.results[0]
        try:
            rdict["document"] = rxml.find(self._add_vers_ns('document')).text
            rdict["revisions"] = []
            revisions = rxml.find(self._add_vers_ns('revisions'))
            for revision in revisions.getchildren():
                rentry = {}
                rentry['revision'] = int(revision.attrib['rev'])
                rentry['date'] = revision.find(self._add_vers_ns('date')).text
                rentry['user'] = revision.find(self._add_vers_ns('user')).text
                rdict["revisions"].append(rentry)
        except AttributeError:
            raise RuntimeError("Could not extract versioning information for"
                               "item at path {}".format(path))

        return rdict

    def _add_vers_ns(self, element):
        return "{" + self.vers_namespace + "}" + element

    def save_item(self, domain, item, item_xml, overwrite=False):
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
        :param item: the items name
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

        # try to save the item xml, if overwrite is set to True we remove
        # the versioning specific attributes first.
        if overwrite:
            item_xml = self._make_xml_if_needed(item_xml)
            versioning_attrs = ['key', 'revision', 'path']
            for attr in versioning_attrs:
                nsattr = self._add_vers_ns(attr)
                if nsattr in item_xml.attrib:
                    item_xml.attrib.pop(nsattr)

        # check if the xml we got passed is an etree or a string. In the latter
        # case convert it
        item_xml = self._make_str_if_needed(item_xml)

        path = "{}/{}/{}".format(self.root, domain, item)
        success = False
        try:
            success = self.dbhandle.load(item_xml, path)
        except ExistDBException:
            success = False

        res_xml = None
        if success:
            res_xml = self._make_str_if_needed(item_xml)
        elif self.dbhandle.hasCollection(path):
            res_xml = self.dbhandle.getDoc(path).decode('utf-8')
            res_xml = self._make_str_if_needed(res_xml)

        meta = {}
        meta["versioning_info"] = self.get_versioning_info(path)
        meta['current_xml'] = res_xml
        return (success, meta)

    def copy_item(self, domain, target_domain, item, target_item):
        """
        Copies item of item_type from domain to target_domain and target_item
        :param domain: the originating domain
        :param target_domain: the target_domain
        :param item: the original item
        :param target_item: the target item
        :return: a string representation of the copy's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if copying failed otherwise.
                AttributeError if a non-supported type is passed
        """

        # create domain if necessary
        if not self.domain_exists(target_domain):
            self.add_domain(target_domain)

        path = "{}/{}".format(self.root, domain)
        target_path = "{}/{}".format(self.root, target_domain)
        return_path = None

        # perform the copy
        # if item names stay the same we can directly copy
        query = None
        if item == target_item:
            query = ('xmldb:copy("{}", "{}", "{}")'
                     .format(path, target_path, item))
            return_path = "{}/{}".format(target_item, item)
        # if they don't match we copy to a temporary, assured unique and then
        # rename
        else:
            query = ('xmldb:copy("{0}", "{1}", "{2}"),'
                     'xmldb:rename("{1}", "{2}", "{3}")'
                     .format(path, target_path, item, target_item))
            return_path = "{}/{}".format(target_path, target_item)

        self.dbhandle.query(query)
        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def rename_item(self, domain, item, target_item):
        """
        Renames item of item_type from in domain to target_item
        :param domain: the originating domain
        :param item: the original item
        :param target_item: the target item
        :return: a string representation of the renamed item's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if renaming failed otherwise.
                AttributeError if a non-supported type is passed
        """
        path = "{}/{}".format(self.root, domain)

        # perform the rename
        query = ('xmldb:rename("{0}", "{1}", "{2}")'
                 .format(path, item, target_item))

        return_path = "{}/{}".format(path, target_item)

        self.dbhandle.query(query)

        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def move_item(self, domain, target_domain, item):
        """
        Moves item of item_type from domain to target_domain
        :param domain: the originating domain
        :param target_domain: the target_domain
        :param item: the original item
        :return: a string representation of the moved item's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if copying failed otherwise.
                AttributeError if a non-supported type is passed
        """

        # create domain if necessary
        if not self.domain_exists(target_domain):
            self.add_domain(target_domain)

        path = "{}/{}".format(self.root, domain)
        target_path = "{}/{}".format(self.root, target_domain)

        # perform the move
        query = ('xmldb:move("{0}", "{1}", "{2}")'
                 .format(path, target_path, item))

        return_path = "{}/{}".format(target_path, item)

        self.dbhandle.query(query)

        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def load_item(self, domain, item, revision=None):
        """
        Load an item of `item_type` from `domain`
        :param domain: a domain to load from
        :param item: the name of the item to load
        :param revision: optional revision number of item, use None if latest.
        :return:
        """
        path = "{}/{}/{}".format(self.root, domain, item)
        if revision is None:
            # simple interface
            try:
                item = self.dbhandle.getDoc(path).decode('utf-8')
            except ExistDBException:
                return ""

        else:
            query = """
            xquery version "3.0";
            import module namespace v="{vnamespace}";
            return v:doc(doc('{path}'), {revision})
            """.format(vnamespace=self.vers_namespace, path=path,
                       revision=revision)
            try:
                item = self.dbhandle.query(query).results[0]
            except ExistDBException:
                return ""

        item = self._make_xml_if_needed(item)
        # add versioning info
        v_info = self.get_versioning_info(path)
        last_rev = 0
        if len(v_info["revisions"]) > 0:
            last_rev = v_info["revisions"][-1]['revision']
        # version filter expects also a key, this will also protect from
        # overwriting versions.
        key = hex(int(round(time.time() * 1000)))+hex(last_rev)
        etree.register_namespace('v', self.vers_namespace)
        item.attrib[self._add_vers_ns('revision')] = str(last_rev)
        item.attrib[self._add_vers_ns('key')] = key
        item.attrib[self._add_vers_ns('path')] = path
        return self._make_str_if_needed(item)

    def load_multi(self, domain, item_xml_str, list_tags):
        """
        Loads all items found in item_xml_str under children.

        :param domain: the domain to load from
        :param item_xml_str: the xml of the container item, at its root
        :param list_tags: tags which identify children to load
        :return:xml string of the loaded items
        """

        # gather information on revisions and uids
        xml = self._make_xml_if_needed(item_xml_str)

        revisions = []
        uuids = []

        for tag in list_tags:
            subs = xml.find(tag)
            for elem in subs:
                revisions.append(elem.attrib['revision'])
                uuids.append(elem.attrib['uuid'])

        # path to where the possible entries are located
        path = "{}/{}".format(self.root, domain)

        query = """
            xquery version "3.0";
            import module namespace v="{vnamespace}";
            let $revs := {revs}
            let $uuids := {uuids}
            for $c at $i in collection("{path}/?select=*")
            where $c/*/@uuid = $uuids
            return v:doc($c, $revs[index-of($uuids, data($c/*/@uuid))-1])
            """.format(vnamespace=self.vers_namespace, revs=tuple(revisions),
                       uuids=tuple(uuids), path=path)

        try:
            res = self.dbhandle.query(query)
        except ExistDBException:
            return {}

        return {r.attrib['uuid']: self._make_str_if_needed(r)
                for r in res.results}

    def list_items(self, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        query = """
                xquery version "3.0";
                {maybe_let}
                for $c at $i in collection("{root}/{domain}/?select=*")
                {maybe_where}
                {return_stmnt}
                """
        maybe_let, maybe_where = '', ''
        if item_types is not None:
            maybe_let = 'let $item_types := {}'.format(tuple(item_types))
            maybe_where = 'where $c/*/@item_type = $item_types'

        r_names = ('uuid', 'simple_name', 'item_type')
        r_attrs = ' '.join(['{name}="{{$c/*/@{name}}}"'.format(name=n)
                           for n in r_names])

        return_stmnt = 'return <item {} />'.format(r_attrs)

        query = query.format(maybe_let=maybe_let,
                             maybe_where=maybe_where,
                             root=self.root, domain=domain,
                             return_stmnt=return_stmnt)
        try:
            res = self.dbhandle.query(query)
            return [{'uuid': r.attrib['uuid'],
                     'item_type': r.attrib['item_type'],
                     'simple_name': r.attrib['simple_name']}
                    for r in res.results]
        except ExistDBException:
            return []

    def list_domains(self):
        """
        List top level domains in database
        :return:
        """

        query = """xquery version "3.0";
                xmldb:get-child-collections("{}")""".format(self.root)

        try:
            res = self.dbhandle.query(query)
            return [r.text for r in res.results]
        except ExistDBException:
            return []
