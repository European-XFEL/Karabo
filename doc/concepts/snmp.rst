************
SNMP Devices
************

Simple Network Management Protocol (SNMP) is an Internet-standard protocol for
collecting and organizing information about managed devices on IP networks and
for modifying that information to change device behavior.

SNMP is widely used in network management systems to monitor network-attached
devices for conditions that warrant administrative attention. SNMP exposes
management data in the form of variables on the managed systems, which describe
the system configuration. These variables can then be queried (and sometimes
set) by managing applications.

SNMP is a component of the Internet Protocol Suite as defined by the Internet
Engineering Task Force (IETF). It consists of a set of standards for network
management, including an application layer protocol, a database schema, and
a set of data objects.

In Karabo, SNMP devices may be accessed through the snmpapi dependency, which
includes both a C++ library alongside Python bindings.

The SNMP connection should be established in the devices initialization method:

.. code-block:: Python

    from snmpapi import Connection

    #...

    def initialization(self):
        # SNMP starting
        snmpconf = Hash()
        snmpconf['hostname'] = self.get("hostname")
        snmpconf['port'] = self.get("snmpPort")
        snmpconf['community'] = "guru"
        snmpconf['version'] = "2"
        self.connection = Connection.create("Snmp", snmpconf)
        self.channel = self.connection.start()

The channel object then provides access to the SNMP device with the following
methods:

.. function:: get(self, arglist):

    where ``arglist`` is a list of OIDs or a tuple of OIDs or one OID as a string.
    It returns a Hash object of (OID, value) items.


    .. code-block:: Python

        # this example woks if 'snmpd' daemon is working in your system
        from karabo.snmpapi import *
        connection = Connection.create("Snmp", Hash())
        channel = connection.start()
        # Use string ...
        h = channel.get('.1.3.6.1.2.1.4.1.0')
        # or list ...
        h = channel.get(['.1.3.6.1.2.1.4.1.0', '.1.3.6.1.2.1.4.2.0'])
        # or tuple ...
        h = channel.get(('.1.3.6.1.2.1.4.1.0', '.1.3.6.1.2.1.4.2.0',))

.. function:: getbulk(self, arglist):

    where ``arglist`` is a list of OIDs or a tuple of OIDs or one OID as a string.
    It returns Hash object of (OID, value) items.

    .. code-block:: Python

        # this example woks if 'snmpd' daemon is working in your system
        from karabo.snmpapi import *
        connection = Connection.create("Snmp", Hash())
        channel = connection.start()
        # Use string ...
        h = channel.getbulk('.1.3.6.1.2.1.4')
        # or list ...
        h = channel.getbulk(['.1.3.6.1.2.1.4', '.1.3.6.1.2.1.4.31'])
        # or tuple ...
        h = channel.getbulk(('.1.3.6.1.2.1.4', '.1.3.6.1.2.1.4.31',))
        coonection.stop()

.. function:: set(self, hash):

    where ``hash`` is a Hash of (OID, value).
    It returns Hash object of (OID, value) items that were set successfully.

.. function:: walk(self, arg):

    where ``arg`` is a OID string below which the OID hierarchy should be walked
    through. It returns the hierarchy as a Hash object

    .. code-block:: Python

        # this example woks if 'snmpd' daemon is working in your system
        from karabo.snmpapi import *
        connection = Connection.create("Snmp", Hash())
        channel = connection.start()
        h = channel.walk('.1.3.6.1.2.1.5')
        flat = Hash()
        h.flatten(flat)
        print flat            # flat "view"



Instead of directly accessing OIDs he SNMP object identifiers may be added
to Karabo's expected parameters as an alias and mapped to Karabo properties
as follows:

.. code-block:: Python

     INT32_ELEMENT(expected).key("sysMainSwitch").alias(".1.3.6.1.4.1.19947.1.1.1.0")
                .tags("slow")
                .displayedName("Crate switch")
                .description("Crate switch status can be ON(1) or OFF(0)")
                .readOnly()
                .commit(),

which resolve to SNMP using e.g.

.. code-block:: Python

    def snmpGet(self, arg):
        with self._channelLock:
            schema = self.fullSchema
            result = Hash()
            if type(arg) is str:
                h = self.channel.get(schema.getAliasFromKey(arg))
            elif type(arg) is tuple or type(arg) is list:
                alist = []
                for key in arg:
                    alias = schema.getAliasFromKey(key)
                    alist.append(alias)
                h = self.channel.get(alist)
            else:
                raise RuntimeError("Argument type: " + type(arg) + " is not supported.")
            # convert resulting Hash of aliases to "normal" Hash
            for a in h.getPaths():
                if schema.aliasHasKey(a):
                    key = schema.getKeyFromAlias(a)
                    result[key] = h.getAs(a, schema.getValueType(key), '/')
            return result

    def smnpSet(self, *args):
        with self._channelLock:
            schema = self.fullSchema
            result = Hash()
            pars = tuple(args)
            print("pars", len(pars))
            if len(pars) == 2:
                key, value = pars
                if type(key) is not str:
                    raise RuntimeError("First argument is not a string")
                alias = schema.getAliasFromKey(key)
                h = Hash()
                h.setAs(alias, value, schema.getValueType(key), '/')
                hash = self.channel.set(h)
                for a in hash.getPaths():
                    if schema.aliasHasKey(a):
                        key = schema.getKeyFromAlias(a)
                        result[key] = hash.getAs(a, schema.getValueType(key), '/')
            elif len(pars) == 1:
                hash = pars[0]
                if type(hash) is not Hash:
                    raise RuntimeError("The argument is not Hash")

                # build hash of mpod snmp commands
                h = Hash()
                for key in hash.getPaths():
                    #print("key", key)
                    if schema.keyHasAlias(key):
                        a = schema.getAliasFromKey(key)
                        #print("alias", a)
                        h.setAs(a, hash[key], schema.getValueType(key), '/')
                # snmp commands only
                if not h.empty():
                    hash = self.channel.set(h)
                    for a in hash.getPaths():
                        if schema.aliasHasKey(a):
                            key = schema.getKeyFromAlias(a)
                            result[key] = hash.getAs(a, schema.getValueType(key), '/')
            else:
                raise RuntimeError("Wrong number of args.  Expected '1' or '2', '" + len(pars) + "' given.")
            return result