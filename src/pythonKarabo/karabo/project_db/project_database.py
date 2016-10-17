import os
import time
from contextlib import ContextDecorator
from eulexistdb import db
from eulexistdb.exceptions import ExistDBException
from lxml import etree

from .util import assure_running
from .dbsettings import DbSettings


class ProjectDatabase(ContextDecorator):

    known_xml_types = ['projects', 'scenes', 'macros', 'configs',
                       'device_servers']
    known_non_xml_types = ['resources']
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

        # for a new domain we need to create the projects, scenes, macros,
        # configuration, device_servers, and resources collections
        sub_colls = self.known_xml_types + self.known_non_xml_types

        success = True
        for coll in sub_colls:
            spath = "{}/{}".format(path, coll)
            success &= self.dbhandle.createCollection(spath)

        if not success:
            raise RuntimeError("Failed to create sub collections in domain {}"
                               .format(path))

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
            return etree.tostring(xml_rep, pretty_print=True,
                                  encoding='UTF-8', xml_declaration=False)\
                                  .decode("utf-8")
        raise AttributeError("Cannot handle type {}".format(type(xml_rep)))

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
                    id: the revision id
                    date: the date this revision was added
                    user: the user that added this revision
        :raises: ExistDBException if user is not authorized for this
                 transaction or the versioning query to the database failed
                 otherwise.
                 RuntimeError: if versioning information could not be extracted
                 from the query results.
        """

        query = ("import module namespace v=\"{}\";"
                 "v:history(doc(\"{}\"))".format(self.vers_namespace, path))
        result = self.dbhandle.query(query)
        if result.count != 1 or result.hits != 1:
            raise RuntimeError("Expected exactly one result when querying"
                               "version information for {}, but got {} hits"
                               "and {} count"
                               .format(path, result.hits, result.count))

        # process result
        rdict = dict()
        rxml = result.results[0]
        try:
            rdict["document"] = rxml.find(self._add_vers_ns('document')).text
            rdict["revisions"] = []
            for revision in rxml.find(self._add_vers_ns('revisions'))\
                    .getchildren():
                rentry = dict()
                rentry['id'] = int(revision.attrib['rev'])
                rentry['date'] = revision.find(self._add_vers_ns('date')).text
                rentry['user'] = revision.find(self._add_vers_ns('user')).text
                rdict["revisions"].append(rentry)
        except AttributeError:
            raise RuntimeError("Could not extract versioning information for"
                               "item at path {}".format(path))

        return rdict

    @classmethod
    def _add_vers_ns(cls, element):
        return "{"+cls.vers_namespace+"}"+element

    @classmethod
    def _check_for_known_xml_type(cls, item_type):
        if item_type not in cls.known_xml_types:
            raise AttributeError("Type {} is not of the known xml types: {}"
                                 .format(item_type, cls.known_xml_types))

    def _save_item(self, item_type, domain, item_name, item_xml,
                   overwrite=False):
        """
        Saves a item xml file into the domain. It will
        create a new entry if the item does not exist yet, or create a new
        version of the item if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        item file is returned

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param item_type: the type of item. Can be any of the following:
                          'projects', 'scenes', 'macros', 'configuration',
                          'device_servers'
        :param domain: the domain under which this item is to be stored
        :param item_name: the items name
        :param item_xml: the xml file containing the item information
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
        # check if type is known to us
        self._check_for_known_xml_type(item_type)

        # create domain if necessary
        if not self.domain_exists(domain):
            self.add_domain(domain)

        # try to save the item xml, if overwrite is set to True we remove
        # the versioning specific attributes first.
        if overwrite:
            versioning_attrs = ['key', 'revision', 'path']
            for attr in versioning_attrs:
                nsattr = '{http://exist-db.org/versioning}'+attr
                if nsattr in item_xml.attrib:
                    item_xml.attrib.pop(nsattr)

        # check if the xml we got passed is an etree or a string. In the latter
        # case convert it
        item_xml = self._make_str_if_needed(item_xml)

        path = "{}/{}/{}/{}".format(self.root, domain, item_type, item_name)
        success = False
        try:
            success = self.dbhandle.load(item_xml, path)
        except ExistDBException:
            success = False

        res_xml = None
        if success:
            res_xml = self._make_str_if_needed(item_xml)
        elif self.dbhandle.hasCollection(path):
                res_xml = self._make_str_if_needed(self.dbhandle.getDoc(path)
                                                   .decode('utf-8'))

        meta = dict()
        meta["versioning_info"] = self.get_versioning_info(path)
        meta['current_xml'] = res_xml
        return (success, meta)

    def save_project(self, domain, project, xml, overwrite=False):
        """
        Saves a project xml file into the domain. It will
        create a new entry if the project does not exist yet, or create a new
        version of the project if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        project file is returned

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this project is to be stored
        :param project: the project's name
        :param xml: the xml file containing the project information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the project.


        :raises: ExistDBException if user is not authorized for this query.
            RuntimeError if project saving failed otherwise.
            AttributeError if a non-supported type is passed
        """
        return self._save_item('projects', domain, project, xml, overwrite)

    def save_scene(self, domain, scene, xml, overwrite=False):
        """
        Saves a scene xml file into the domain. It will
        create a new entry if the scene does not exist yet, or create a new
        version of the scene if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        scene file is returned

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this scene is to be stored
        :param scene: the scene's name
        :param xml: the xml file containing the scene information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the scene.


        :raises: ExistDBException if user is not authorized for this query.
            RuntimeError if scene saving failed otherwise.
            AttributeError if a non-supported type is passed
        """
        return self._save_item('scenes', domain, scene, xml, overwrite)

    def save_config(self, domain, config, xml, overwrite=False):
        """
        Saves a config xml file into the domain. It will
        create a new entry if the config does not exist yet, or create a new
        version of the config if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        config file is returned

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this config is to be stored
        :param config: the config's name
        :param xml: the xml file containing the config information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the config.


        :raises: ExistDBException if user is not authorized for this query.
            RuntimeError if scene config failed otherwise.
            AttributeError if a non-supported type is passed
        """
        return self._save_item('configs', domain, config, xml, overwrite)

    def save_device_server(self, domain, device_server, xml, overwrite=False):
        """
        Saves a device server xml file into the domain. It will
        create a new entry if the device server does not exist yet, or create a
        new version of the device server if it does exist. In case of a
        versioning conflict the update will fail and the most current version
        of the config file is returned

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this config is to be stored
        :param device_server: the device server's name
        :param xml: the xml file containing the device server information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the device server.


        :raises: ExistDBException if user is not authorized for this query.
            RuntimeError if scene config failed otherwise.
            AttributeError if a non-supported type is passed
        """
        return self._save_item('device_servers', domain, device_server, xml,
                               overwrite)

    def _copy_item(self, item_type, domain, target_domain, item, target_item):
        """
        Copies item of item_type from domain to target_domain and target_item
        :param item_type: type of item
        :param domain: the originating domain
        :param target_domain: the target_domain
        :param item: the original item
        :param target_item: the target item
        :return: a string representation of the copy's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if copying failed otherwise.
                AttributeError if a non-supported type is passed
        """
        # check if type is known to us
        self._check_for_known_xml_type(item_type)

        # create domain if necessary
        if not self.domain_exists(target_domain):
            self.add_domain(target_domain)

        path = "{}/{}/{}".format(self.root, domain, item_type)
        target_path = "{}/{}/{}".format(self.root, target_domain, item_type)
        return_path = None

        # perform the copy
        # if item names stay the same we can directly copy
        query = None
        if item == target_item:
            query = ("xmldb:copy(\"{}\", \"{}\", \"{}\")"
                     .format(path, target_path, item))
            return_path = "{}/{}".format(target_item, item)
        # if they don't match we copy to a temporary, assured unique and then
        # rename
        else:
            query = ("xmldb:copy(\"{0}\", \"{1}\", \"{2}\"),"
                     "xmldb:rename(\"{1}\", \"{2}\", \"{3}\")"
                     .format(path, target_path, item, target_item))
            return_path = "{}/{}".format(target_path, target_item)

        self.dbhandle.query(query)
        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def _rename_item(self, item_type, domain, item, target_item):
        """
        Renames item of item_type from in domain to target_item
        :param item_type: type of item
        :param domain: the originating domain
        :param item: the original item
        :param target_item: the target item
        :return: a string representation of the renamed item's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if renaming failed otherwise.
                AttributeError if a non-supported type is passed
        """
        # check if type is known to us
        self._check_for_known_xml_type(item_type)

        path = "{}/{}/{}".format(self.root, domain, item_type)

        # perform the rename
        query = ("xmldb:rename(\"{0}\", \"{1}\", \"{2}\")"
                 .format(path, item, target_item))

        return_path = "{}/{}".format(path, target_item)

        self.dbhandle.query(query)

        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def _move_item(self, item_type, domain, target_domain, item):
        """
        Moves item of item_type from domain to target_domain
        :param item_type: type of item
        :param domain: the originating domain
        :param target_domain: the target_domain
        :param item: the original item
        :return: a string representation of the moved item's xml
        :raises: ExistDBException if user is not authorized for this query.
                RuntimeError if copying failed otherwise.
                AttributeError if a non-supported type is passed
        """
        # check if type is known to us
        self._check_for_known_xml_type(item_type)

        # create domain if necessary
        if not self.domain_exists(target_domain):
            self.add_domain(target_domain)

        path = "{}/{}/{}".format(self.root, domain, item_type)
        target_path = "{}/{}/{}".format(self.root, target_domain, item_type)

        # perform the move
        query = ("xmldb:move(\"{0}\", \"{1}\", \"{2}\")"
                 .format(path, target_path, item))

        return_path = "{}/{}".format(target_path, item)

        self.dbhandle.query(query)

        return self.dbhandle.getDoc(return_path).decode('utf-8')

    def load_item(self, item_type, domain, item, revision=None):
        """
        Load an item of item_type from domain
        :param item_type: the type of the item as per the list of types
        :param domain: a domain to load from
        :param item: the name of the item to load
        :param revision: optional revision number of item, use None if latest.
        :return:
        """
        path = "{}/{}/{}/{}".format(self.root, domain, item_type, item)
        if revision is None:
            # simple interface
            item = self.dbhandle.getDoc(path).decode('utf-8')
        else:
            query = """
            xquery version "3.0";
            import module namespace v="http://exist-db.org/versioning";
            return v:doc(doc('{path}'), {revision})
            """.format(path=path, revision=revision)
            item = self.dbhandle.query(query).results[0]

        item = self._make_xml_if_needed(item)
        # add versioning info
        v_info = self.get_versioning_info(path)
        last_rev = v_info["revisions"][-1]['id'] \
            if len(v_info["revisions"]) > 0 \
            else 0
        # version filter expects also a key, this will also protect from
        # overwriting versions.
        key = hex(int(round(time.time() * 1000)))+hex(last_rev)
        etree.register_namespace('v', 'http://exist-db.org/versioning')
        item.attrib['{http://exist-db.org/versioning}revision'] = str(last_rev)
        item.attrib['{http://exist-db.org/versioning}key'] = key
        item.attrib['{http://exist-db.org/versioning}path'] = path
        return self._make_str_if_needed(item)

    def _load_multi(self, domain, item_xml_str, list_tag):
        """
        Loads all items found in item_xml_str under listTag. Here list_tag
        needs to correspond to one of the known item types
        :param domain:the domain to load from
        :param item_xml_str:the xml of the container item, should contain a tag
                list_tag which holds the entries to load
        :param list_tag: the tag name that contains the list
        :return:xml string of the loaded items
        """

        # gather information on revisions and uids
        xml = self._make_xml_if_needed(item_xml_str)
        revisions = []
        uids = []
        configs = xml.find(list_tag)
        for elem in configs:
            revisions.append(elem.attrib['revision'])
            uids.append(elem.attrib['uid'])

        # path to where the possible entries are located
        path = "{}/{}/{}".format(self.root, domain, list_tag)

        query = """
            xquery version "3.0";
            import module namespace v="http://exist-db.org/versioning";
            let $revs := {revs}
            let $uids := {uids}
            for $c at $i in collection("{path}/?select=*")
            where $c//@uid = $uids
            return v:doc($c, $revs[index-of($uids, data($c//@uid))-1])
            """.format(revs=tuple(revisions), uids=tuple(uids), path=path)

        res = self.dbhandle.query(query)
        return [self._make_str_if_needed(r) for r in res.results]
