from abc import abstractmethod, ABC
from contextlib import ContextDecorator
from time import gmtime, strftime

from lxml import etree

from .util import ProjectDBError


DATE_FORMAT = '%Y-%m-%d %H:%M:%S'


class HandleABC(ABC):
    """Database Handle ABC

    it enforces only the basic "load/save" functionality.
    Queries are left to implementation in the derived class.
    """
    @abstractmethod
    def hasCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def removeCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def createCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def hasDocument(self, path):
        raise NotImplementedError

    @abstractmethod
    def load(self, data, path):
        raise NotImplementedError

    @abstractmethod
    def getDoc(self, name):
        raise NotImplementedError

    @abstractmethod
    def getDocument(self, name):
        raise NotImplementedError


class DatabaseBase(ContextDecorator):
    root = None

    def __init__(self):
        self._dbhandle = None

    @property
    def dbhandle(self):
        return self._dbhandle

    @dbhandle.setter
    def dbhandle(self, value):
        assert isinstance(value, HandleABC)
        self._dbhandle = value

    def onEnter(self):
        """"To Be implemented by subclasses

        returns an object subclass of `HandleABC` which is set to the
        `self.dbhandle` object attribute in the `__enter__` conte
        """

    def path(self, domain, uuid):
        # XXX: Add a '_0' suffix to keep old code from wetting its pants
        return "{}/{}/{}_0".format(self.root, domain, uuid)

    def __enter__(self):
        """Obtain a database handle.

        sanity checks on the database should be implemented in this function
        """
        self.dbhandle = self.onEnter()
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

    def domain_exists(self, domain):
        """
        Checks if a given domain exists.
        :param domain: the domain to check
        :return: True if it exists, false otherwise
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
        :raises: ProjectDBError on Handle failure,
                 or RuntimeError if domain creation failed otherwise
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
        if isinstance(xml_rep, bytes):
            xml_rep = xml_rep.decode('utf-8')
        if isinstance(xml_rep, str):
            try:
                return etree.fromstring(xml_rep)
            except etree.XMLSyntaxError:
                raise ValueError("XML syntax error encountered while parsing!")

        raise ValueError(f"Cannot handle type {type(xml_rep)}: {xml_rep}")

    @staticmethod
    def _make_str_if_needed(xml_rep):
        """
        Returns a string representation of xml_rep
        :param xml_rep: the xml
        :return: a string representation of xml_rep
        :raises: ValueError if the object passed is not of type str or type
                 etree.ElementBase
        """
        if isinstance(xml_rep, bytes):
            xml_rep = xml_rep.decode('utf-8')
        if isinstance(xml_rep, str):
            return xml_rep
        if isinstance(xml_rep, etree._Element):
            xml_bytes = etree.tostring(xml_rep, pretty_print=True,
                                       encoding="unicode",
                                       xml_declaration=False)
            return xml_bytes

        raise ValueError("Cannot handle type {}".format(type(xml_rep)))

    def sanitize_database(self, domain):
        """Optional Method

        Migrates a DB to the latest sane configuration"""

    def get_configurations_from_device_name(self, domain, device_id):
        """Returns a list of configurations for a given device

        To be implemented in the derived class

        :param domain: DB domain
        :param instance_id: instance id of the device
        :return: a list of dicts:
            [{"configid": uuid of the configuration,
              "instanceid": device instance uuid in the DB},
              ...
            ]
        """
        raise NotImplementedError

    def get_projects_from_device(self, domain, uuid):
        """
        Returns the projects which contain a device instance with a given uuid

        To be implemented in the derived class

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """
        raise NotImplementedError

    def get_projects_with_conf(self, domain, device_id):
        """
        Returns a dict with projects and active configurations from a device
        name.

        :param domain: DB domain
        :param device_id: the device to return the information for.
        :return: a dict:
            {"project name": configuration uuid,
             ...}
        """
        configs = self.get_configurations_from_device_name(domain,
                                                           device_id)
        projects = dict()
        for config in configs:
            instance_id = config["instanceid"]
            for project in self.get_projects_from_device(domain,
                                                         instance_id):
                projects[project] = config["configid"]
        return projects

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

        item_xml = self._make_str_if_needed(item_tree)
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