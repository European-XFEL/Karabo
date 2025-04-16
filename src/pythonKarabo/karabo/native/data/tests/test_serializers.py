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
import os
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase, main
from zlib import adler32

from ..bin_reader import decodeBinary, decodeBinaryPos
from ..bin_writer import encodeBinary, writeBinary
from ..hash import Hash, HashList
from ..schema import Schema
from ..xml_reader import decodeXML, loadFromFile
from ..xml_writer import encodeXML, saveToFile, writeXML

# The following XML was generated using the C++ API `saveToFile`
BOUND_HASH_XML = """<?xml version="1.0"?>
<root KRB_Artificial="" KRB_Type="HASH">
  <akey KRB_Type="STRING">aval</akey>
  <another KRB_Type="HASH">
    <nested KRB_Type="DOUBLE">1.618000000000000</nested>
  </another>
</root>
"""
# The following XML is what the middlelayer API generates
MDL_HASH_XML = '<root KRB_Artificial=""><akey KRB_Type="STRING" >aval</akey><another KRB_Type="HASH" ><nested KRB_Type="DOUBLE" >1.618</nested></another></root>'  # noqa

# XML generated using the C++ api 'saveToFile' for a vector of hash.
BOUND_VECTOR_HASH_XML = """<?xml version="1.0"?>
<root KRB_Artificial="" KRB_Type="HASH">
    <KRB_Sequence KRB_Type="VECTOR_HASH">
       <KRB_Item>
           <e1 KRB_Type="STRING">ab.KRB_NEWLINE.c3</e1>
           <e2 alarmCondition="KRB_STRING:none" KRB_Type="BOOL">0</e2>
           <e3 alarmCondition="KRB_STRING:none" KRB_Type="INT32">36</e3>
           <e4 alarmCondition="KRB_STRING:none" KRB_Type="FLOAT">2.9511</e4>
           <e5 alarmCondition="KRB_STRING:none" KRB_Type="DOUBLE">3.7035</e5>
        </KRB_Item>
    </KRB_Sequence>
</root>
"""

# XML generated using the C++ api for a schema with a TableElement.
BOUND_TABLE_SCHEMA_XML = """<?xml version="1.0"?>
    <root KRB_Artificial="" KRB_Type="HASH">
    <table displayedName="KRB_STRING:Table property"
      description="KRB_STRING:Table containing one node."
      assignment="KRB_INT32:0"
      defaultValue="KRB_VECTOR_HASH:_attr_root_table_defaultValue"
      accessMode="KRB_INT32:4" nodeType="KRB_INT32:0"
      leafType="KRB_INT32:0" displayType="KRB_STRING:Table"
      valueType="KRB_STRING:VECTOR_HASH"
      rowSchema="KRB_SCHEMA:_attr_root_table_rowSchema"
      requiredAccessLevel="KRB_INT32:1"
      overwriteRestrictions=
        "KRB_VECTOR_BOOL:0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0"
      KRB_Type="INT32">
        <_attr_root_table_defaultValue>
            <_attr_root_table_defaultValue_value KRB_Type="VECTOR_HASH">
                <KRB_Item>
                    <e1 KRB_Type="STRING">abc</e1>
                    <e2 alarmCondition="KRB_STRING:none" KRB_Type="BOOL">1</e2>
                    <e3 alarmCondition="KRB_STRING:none" KRB_Type="INT32">
                        12
                    </e3>
                    <e4 alarmCondition="KRB_STRING:none" KRB_Type="FLOAT">
                        0.9837
                    </e4>
                    <e5 alarmCondition="KRB_STRING:none" KRB_Type="DOUBLE">
                        1.2345
                    </e5>
                </KRB_Item>
                <KRB_Item>
                    <e1 KRB_Type="STRING">xyz</e1>
                    <e2 alarmCondition="KRB_STRING:none" KRB_Type="BOOL">0</e2>
                    <e3 alarmCondition="KRB_STRING:none" KRB_Type="INT32">
                        42
                    </e3>
                    <e4 alarmCondition="KRB_STRING:none" KRB_Type="FLOAT">
                        2.33333
                    </e4>
                    <e5 alarmCondition="KRB_STRING:none" KRB_Type="DOUBLE">
                        7.77777
                    </e5>
                </KRB_Item>
            </_attr_root_table_defaultValue_value>
        </_attr_root_table_defaultValue>
        <_attr_root_table_rowSchema>
            <_attr_root_table_rowSchema_value KRB_Type="SCHEMA">
              :&lt;?xml version="1.0"?&gt;&lt;root KRB_Artificial="" KRB_Type="HASH"&gt;&lt;e1 daqPolicy="KRB_INT32:-1" displayedName="KRB_STRING:E1" description="KRB_STRING:E1 property" assignment="KRB_INT32:0" defaultValue="KRB_STRING:E1" accessMode="KRB_INT32:4" nodeType="KRB_INT32:0" leafType="KRB_INT32:0" valueType="KRB_STRING:STRING" requiredAccessLevel="KRB_INT32:1" KRB_Type="INT32"&gt;0&lt;/e1&gt;&lt;e2 daqPolicy="KRB_INT32:-1" displayedName="KRB_STRING:E2" description="KRB_STRING:E2 property" accessMode="KRB_INT32:4" assignment="KRB_INT32:0" defaultValue="KRB_BOOL:0" nodeType="KRB_INT32:0" leafType="KRB_INT32:0" valueType="KRB_STRING:BOOL" requiredAccessLevel="KRB_INT32:1" KRB_Type="INT32"&gt;0&lt;/e2&gt;&lt;e3 daqPolicy="KRB_INT32:-1" displayedName="KRB_STRING:E3" description="KRB_STRING:E3 property" accessMode="KRB_INT32:4" assignment="KRB_INT32:0" defaultValue="KRB_INT32:77" nodeType="KRB_INT32:0" leafType="KRB_INT32:0" valueType="KRB_STRING:INT32" requiredAccessLevel="KRB_INT32:1" KRB_Type="INT32"&gt;0&lt;/e3&gt;&lt;e4 daqPolicy="KRB_INT32:-1" displayedName="KRB_STRING:E4" description="KRB_STRING:E4 property" assignment="KRB_INT32:0" defaultValue="KRB_FLOAT:3.1415" accessMode="KRB_INT32:4" nodeType="KRB_INT32:0" leafType="KRB_INT32:0" valueType="KRB_STRING:FLOAT" requiredAccessLevel="KRB_INT32:1" KRB_Type="INT32"&gt;0&lt;/e4&gt;&lt;e5 daqPolicy="KRB_INT32:-1" displayedName="KRB_STRING:E5" description="KRB_STRING:E5 property" assignment="KRB_INT32:0" defaultValue="KRB_DOUBLE:2.78" accessMode="KRB_INT32:4" nodeType="KRB_INT32:0" leafType="KRB_INT32:0" valueType="KRB_STRING:DOUBLE" requiredAccessLevel="KRB_INT32:1" KRB_Type="INT32"&gt;0&lt;/e5&gt;&lt;/root&gt;</_attr_root_table_rowSchema_value>
        </_attr_root_table_rowSchema>0
    </table>
    </root>
"""  # noqa

# XML generated using the C++ api for a schema with a TableElement before the
# addition of support to VectorHash and Schema attributes (in MR !3592).
BOUND_TABLE_SCHEMA_LEGACY_XML = """<?xml version="1.0"?>
    <root KRB_Artificial="" KRB_Type="HASH">
        <table displayedName="KRB_STRING:Table property"
          description="KRB_STRING:Table containing one node."
          assignment="KRB_INT32:0"
          nodeType="KRB_INT32:0"
          leafType="KRB_INT32:0"
          displayType="KRB_STRING:Table"
          valueType="KRB_STRING:VECTOR_HASH"
          requiredAccessLevel="KRB_INT32:1"
          accessMode="KRB_INT32:4"
          rowSchema="KRB_SCHEMA:Schema Object"
          defaultValue="KRB_VECTOR_HASH:'e1' =&gt; abc STRING&#10;'e2' alarmCondition=&quot;none&quot; =&gt; 1 BOOL&#10;'e3' alarmCondition=&quot;none&quot; =&gt; 12 INT32&#10;'e4' alarmCondition=&quot;none&quot; =&gt; 0.9837 FLOAT&#10;'e5' alarmCondition=&quot;none&quot; =&gt; 1.2345 DOUBLE&#10;,'e1' =&gt; xyz STRING&#10;'e2' alarmCondition=&quot;none&quot; =&gt; 0 BOOL&#10;'e3' alarmCondition=&quot;none&quot; =&gt; 42 INT32&#10;'e4' alarmCondition=&quot;none&quot; =&gt; 2.33333 FLOAT&#10;'e5' alarmCondition=&quot;none&quot; =&gt; 7.77777 DOUBLE&#10;"
          KRB_Type="INT32">0
        </table>
    </root>
"""  # noqa

# The following is used accross all tests as template Hash
HASH = Hash('akey', 'aval', 'another', Hash('nested', 1.618))
FILENAME = os.path.join("all", "the", "folders", "hash.xml")


class TestSerializers(TestCase):
    def setUp(self):
        self.test_dir = TemporaryDirectory()
        self.oldcwd = os.path.abspath(os.curdir)
        os.chdir(self.test_dir.name)  # Change the working dir, to not pollute

    def tearDown(self):
        os.chdir(self.oldcwd)
        self.test_dir.cleanup()

    def test_saveToFile(self):
        # Test saving in the current directory
        saveToFile(HASH, 'hash.xml')
        self.assertTrue(os.path.exists('hash.xml'))

        # Test without any sub-directories existing
        saveToFile(HASH, FILENAME)
        self.assertTrue(os.path.exists(FILENAME))

        with open(FILENAME) as fin:
            read_string = fin.read()
        self.assertEqual(read_string, MDL_HASH_XML)

        read_hash = decodeXML(read_string)
        self.assertEqual(read_hash, HASH)

        # Test with sub-directories already existing
        os.remove(FILENAME)
        self.assertFalse(os.path.exists(FILENAME))
        try:
            saveToFile(HASH, FILENAME)
        except Exception:
            self.fail("Failed as directory already exists")
        self.assertTrue(os.path.exists(FILENAME))

        # Test with xml file already existing
        self.assertTrue(os.path.exists(FILENAME))
        try:
            saveToFile(HASH, FILENAME)
        except Exception as e:
            self.fail(f"Failed as file already exists: {e}")

        # Test with non-Hash value
        with self.assertRaises(Exception):
            saveToFile(None, FILENAME)

        # Test with a Path object
        filename = Path('path_hash.xml')
        saveToFile(HASH, filename)
        self.assertTrue(filename.exists())

    def test_loadFromFile(self):
        # Test valid xml
        with open("hash.xml", "w") as fout:
            fout.write(encodeXML(HASH))

        h = loadFromFile("hash.xml")
        self.assertEqual(h, HASH)

        # Test malformatted xml
        with open("bad.xml", "w") as fout:
            fout.write(MDL_HASH_XML[:-5])

        with self.assertRaises(Exception):
            loadFromFile("bad.xml")

    def test_load_bound_hash_xml(self):
        with open("bash.xml", "w") as fout:
            fout.write(BOUND_HASH_XML)

        h = loadFromFile("bash.xml")
        self.assertEqual(h, HASH)

        # Test with a Path object
        filename = Path('bash.xml')
        h = loadFromFile(filename)
        self.assertEqual(h, HASH)

    def test_xml_VectorHash_load_bin(self):
        """Tests binary serialization of vector of hash encoded in XML."""
        vh_fromxml = decodeXML(BOUND_VECTOR_HASH_XML)
        # Vector of Hash from XML is actually a hash with the Vector of
        # Hash as the value of its sole key, 'KRB_Sequence'.
        self.assertEqual(len(vh_fromxml), 1)
        self.assertTrue(vh_fromxml.has('KRB_Sequence'))
        self.assertTrue(isinstance(vh_fromxml['KRB_Sequence'], HashList))
        # Checks that the MDL binary decoder, like the C++/Bound one
        # retrieves a vector of hash directly, not a single keyed hash.
        bin_enc = encodeBinary(vh_fromxml)
        vh = decodeBinary(bin_enc)
        self.assertEqual(len(vh), 1)
        self.assertEqual(vh[0]['e3'], 36)

    def test_xml_BoundSchema_load(self):
        """Tests that a xml for a Bound Schema with vector of hash and
        schema attributes can be succesfully loaded by the MDL xml serializer.
        """
        sch_hash = decodeXML(BOUND_TABLE_SCHEMA_XML)
        self.assertTrue(sch_hash.has('table'))
        self.assertTrue(
            isinstance(sch_hash['table', 'rowSchema'], Schema)
        )
        self.assertTrue(
            isinstance(sch_hash['table', 'defaultValue'], HashList))

    def test_legacy_xml_BoundSchema_load(self):
        """Tests that a legacy xml for a Bound Schema with vector of hash and
        schema attributes can be loaded cleanly by the MDL xml serializer. As
        the legacy xml didn't handle serialization of Schema and VectorHash
        attributes, serializing them as simple strings, their values won't be
        retrieved properly.
        """
        sch_hash = decodeXML(BOUND_TABLE_SCHEMA_LEGACY_XML)
        self.assertTrue(sch_hash.has('table'))
        self.assertTrue(
            isinstance(sch_hash['table', 'rowSchema'], str)
        )
        self.assertEqual(sch_hash['table', 'rowSchema'], 'Schema Object')
        self.assertTrue(
            isinstance(sch_hash['table', 'defaultValue'], str)
        )
        self.assertTrue(sch_hash['table', 'defaultValue'].startswith("'e1'"))

    def test_bin_serialization(self):
        from .test_hash import check_hash, create_hash

        h = create_hash()
        check_hash(h)
        encoded = encodeBinary(h)
        self.assertEqual(adler32(encoded), 1248828222)
        decoded = decodeBinary(encoded)
        check_hash(decoded)

        assert decoded.fullyEqual(h)

        header = Hash('a', 12, 'b.c', 3.1415, 'c.d.e', 'header')
        body = h
        encodedHeader = encodeBinary(header)
        encodedBody = encodeBinary(body)

        m = b''.join([encodedHeader, encodedBody])

        decodedHeader, pos = decodeBinaryPos(m)
        assert pos, len(encodedHeader)
        assert decodedHeader.fullyEqual(header)
        decodedBody = decodeBinary(m[pos:])
        check_hash(decodedBody)
        assert decodedBody.fullyEqual(body)

    def test_xml_serialization(self):
        from .test_hash import check_hash, create_hash

        h = create_hash()
        check_hash(h)
        encoded = encodeXML(h)
        self.assertEqual(adler32(encoded.encode('utf-8')), 3901051482)
        decoded = decodeXML(encoded)
        check_hash(decoded)

        assert decoded.fullyEqual(h)

    def test_write_binary(self):
        from .test_hash import create_hash
        FILENAME = 'test_hash_bin.bin'
        h = create_hash()
        with TemporaryDirectory() as tmpdirname:
            FILENAME = os.path.join(tmpdirname, FILENAME)
            with open(FILENAME, 'wb') as fp:
                writeBinary(h, fp)

            with open(FILENAME, 'rb') as fp:
                decoded = decodeBinary(fp.read())
                assert decoded.fullyEqual(h)

        self.assertFalse(os.path.exists(FILENAME))

    def test_write_xml(self):
        from .test_hash import create_hash
        FILENAME = 'test_hash_xml.xml'
        h = create_hash()
        with TemporaryDirectory() as tmpdirname:
            FILENAME = os.path.join(tmpdirname, FILENAME)
            with open(FILENAME, 'w') as fp:
                writeXML(h, fp)

            with open(FILENAME) as fp:
                decoded = decodeXML(fp.read())
                assert decoded.fullyEqual(h)

        self.assertFalse(os.path.exists(FILENAME))


if __name__ == "__main__":
    main()
