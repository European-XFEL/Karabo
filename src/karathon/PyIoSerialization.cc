/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/io/SchemaXsdSerializer.hh>
#include <karabo/io/SchemaXmlSerializer.hh>
#include <karabo/io/HashXmlSerializer.hh>
#include <karabo/io/HashBinarySerializer.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace bp = boost::python;


namespace karabo {
    namespace io {

        class HashXmlSerializerWrap {

        public:


            static bp::object save(HashXmlSerializer& s, const karabo::util::Hash& hash) {
                std::string archive;
                s.save(hash, archive);
                return bp::object(archive);
            }


            static bp::object load(HashXmlSerializer& s, const std::string& archive) {
                Hash hash;
                s.load(hash, archive);
                return bp::object(hash);
            }
        };

        class SchemaXsdSerializerWrap {

        public:


            static bp::object save(SchemaXsdSerializer& s, const karabo::util::Schema& schema) {
                std::string archive;
                s.save(schema, archive);
                return bp::object(archive);
            }
        };

        class SchemaXmlSerializerWrap {

        public:


            static bp::object save(SchemaXmlSerializer& s, const karabo::util::Schema& schema) {
                std::string archive;
                s.save(schema, archive);
                return bp::object(archive);
            }


            static bp::object load(SchemaXmlSerializer& s, const std::string& archive) {
                Schema schema;
                s.load(schema, archive);
                return bp::object(schema);
            }
        };

        class HashBinarySerializerWrap {

        public:


            static bp::object save(HashBinarySerializer& s, const karabo::util::Hash& hash) {
                std::vector<char> v;
                s.save(hash, v);
                return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(&v[0], v.size())));
            }


            static bp::object load(HashBinarySerializer& s, const bp::object& obj) {
                if (PyByteArray_Check(obj.ptr())) {
                    PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                    size_t size = PyByteArray_Size(bytearray);
                    char* data = PyByteArray_AsString(bytearray);
                    Hash hash;
                    s.load(hash, data, size);
                    return bp::object(hash);
                }
                throw KARABO_PYTHON_EXCEPTION("Python object type is not a bytearray!");
            }
        };
    }
}


void exportPyIoSerialization() {

    bp::docstring_options docs(true, true, false);

    {//exposing karabo::io::HashXmlSerializer
        Hash config("writeDataTypes", true,
                    "readDataTypes", true,
                    "indentation", -1,
                    "xmlns", "http://xfel.eu/config",
                    "prefix", "KRB_",
                    "insertXmlNamespace", true);
        bp::class_< HashXmlSerializer > h("HashXmlSerializer",
                                          "HashXmlSerializer converts Hash object into the string in XML format.  How does this XML string look like "
                                          "depends on configuration parameters (in form of Hash) given to the constructor of serializer.\n  The default"
                                          " configuration contains the following parameters:\n\t\"writeDataTypes\"\t: True\n\t\"readDataTypes\"\t\t"
                                          ": True\n\t\"indentation\"\t\t: -1\n\t\"xmlns\"\t\t\t: \"http://xfel.eu/config\"\n\t\"prefix\"\t\t: \"KRB_\"\n\t"
                                          "\"insertXmlNamespace\"\t: True"
                                          , bp::init<Hash const &>((bp::arg("configuration") = config)));
        h.def("save"
              , &HashXmlSerializerWrap().save
              , (bp::arg("hash"))
              , "Saves the hash into XML string formatted according configuration parameters.  Returns python bytearray object.\nExample:\n\t"
              "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = HashXmlSerializer()\n\tarchive = ser.save(h)\n\tassert archive.__class__.__name__ == 'bytearray'");
        h.def("load"
              , &HashXmlSerializerWrap().load
              , (bp::arg("archive"))
              , "Loads the serialized XML archive and returns new Hash object.\nExample:\n\th = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = HashXmlSerializer()\n\t"
              "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert similar(h, h2)");
    }

    {//exposing karabo::io::SchemaXsdSerializer
        Hash config("indentation", -1, "xmlns", "http://www.w3.org/2001/XMLSchema", "xmlnsa", "http://www.karabo.eu");
        bp::class_< SchemaXsdSerializer > s("SchemaXsdSerializer",
                                            "SchemaXsdSerializer converts Schema object into XSD format.  How does this XSD string look like "
                                            "is influenced by configuration parameters in form of Hash object given to constructor.\nThe default"
                                            " parameter settings are ...\n\t\"indentation\"\t: -1\n\t\"xmlns\"\t\t: \"http://www.w3.org/2001/XMLSchema\""
                                            "\n\t\"xmlnsa\"\t:\"http://www.karabo.eu\""
                                            , bp::init<Hash const &>((bp::arg("configuration") = config)));
        s.def("save"
              , &SchemaXsdSerializerWrap().save
              , (bp::arg("schema"))
              , "Saves the schema into XSD string formatted according configuration parameters.  Returns python bytearray object.\nExample:\n\t"
              "schema = Shape.getSchema('Rectangle')\n\tser = SchemaXsdSerializer()\n\tarchive = ser.save(schema)\n\tassert archive.__class__.__name__ == 'bytearray'");
    }

    {//exposing karabo::io::SchemaXmlSerializer
        Hash config("writeDataTypes", true,
                    "readDataTypes", true,
                    "indentation", -1,
                    "xmlns", "http://xfel.eu/config",
                    "prefix", "KRB_",
                    "insertXmlNamespace", true);
        bp::class_< SchemaXmlSerializer > s("SchemaXmlSerializer",
                                            "SchemaXmlSerializer converts Hash object into the string in XML format.  How does this XML string look like "
                                            "depends on configuration parameters (in form of Hash) given to the constructor of serializer.\n  The default"
                                            " configuration contains the following parameters:\n\t\"writeDataTypes\"\t: True\n\t\"readDataTypes\"\t\t"
                                            ": True\n\t\"indentation\"\t\t: -1\n\t\"xmlns\"\t\t\t: \"http://xfel.eu/config\"\n\t\"prefix\"\t\t: \"KRB_\"\n\t"
                                            "\"insertXmlNamespace\"\t: True"
                                            , bp::init<Hash const &>((bp::arg("configuration") = config)));
        s.def("save"
              , &SchemaXmlSerializerWrap().save
              , (bp::arg("schema"))
              , "Saves the schema into XML string formatted according configuration parameters.  Returns python bytearray object.\nExample:\n\t"
              "schema = Shape.getSchema('Rectangle')\n\tser = SchemaXmlSerializer()\n\tarchive = ser.save(schema)\n\tassert archive.__class__.__name__ == 'bytearray'");

        s.def("load"
              , &SchemaXmlSerializerWrap().load
              , (bp::arg("archive"))
              , "Loads the serialized XML archive and returns new Schema object.\nExample:\n\tschema = Shape.getSchema('Rectangle')\n\tser = SchemaXmlSerializer()\n\t"
              "archive = ser.save(h)\n\tschema2 = ser.load(archive)\n\tassert similar(schema.getParameterHash(), schema2.getParameterHash())");
    }

    {//exposing karabo::io::SchemaXmlSerializer
        bp::class_< HashBinarySerializer > s("HashBinarySerializer",
                                             "HashBinarySerializer converts Hash object into the python bytearray.  No configuration required when creating serializer."
                                             , bp::init<Hash const &>((bp::arg("configuration") = Hash())));
        s.def("save"
              , &HashBinarySerializerWrap().save
              , (bp::arg("hash"))
              , "Saves the hash into bytearray.\nExample:\n\t"
              "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = HashBinarySerializer()\n\tarchive = ser.save(h)\n\tassert archive.__class__.__name__ == 'bytearray'");

        s.def("load"
              , &HashBinarySerializerWrap().load
              , (bp::arg("archive_as_bytearray"))
              , "Loads \"bytearray\" archive and returns new Hash object.\nExample:\n\th = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = HashBinarySerializer()\n\t"
              "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert similar(h, h2)");
    }

}
