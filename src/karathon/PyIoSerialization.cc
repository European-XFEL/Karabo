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
    }
}


void exportPyIoSerialization() {

    {//exposing karabo::io::HashXmlSerializer
        Hash config("writeDataTypes", true,
                    "readDataTypes", true,
                    "indentation", -1,
                    "xmlns", "http://xfel.eu/config",
                    "prefix", "KRB_",
                    "insertXmlNamespace", true);
        bp::class_< HashXmlSerializer > h("HashXmlSerializer", bp::init<Hash const &>((bp::arg("configuration") = config)));
        h.def("save"
              , &HashXmlSerializerWrap().save
              , (bp::arg("hash")));
        h.def("load"
              , &HashXmlSerializerWrap().load
              , (bp::arg("archive")));
    }

    {//exposing karabo::io::SchemaXsdSerializer
        Hash config("indentation", -1, "xmlns", "http://www.w3.org/2001/XMLSchema", "xmlnsa", "http://www.karabo.eu");
        bp::class_< SchemaXsdSerializer > s("SchemaXsdSerializer", bp::init<Hash const &>((bp::arg("configuration") = config)));
        s.def("save"
              , &SchemaXsdSerializerWrap().save
              , (bp::arg("schema")));
    }

    {//exposing karabo::io::SchemaXmlSerializer
        Hash config("writeDataTypes", true,
                    "readDataTypes", true,
                    "indentation", -1,
                    "xmlns", "http://xfel.eu/config",
                    "prefix", "KRB_",
                    "insertXmlNamespace", true);
        bp::class_< SchemaXmlSerializer > s("SchemaXmlSerializer", bp::init<Hash const &>((bp::arg("configuration") = config)));
        s.def("save"
              , &SchemaXmlSerializerWrap().save
              , (bp::arg("schema")));
        s.def("load"
              , &SchemaXmlSerializerWrap().load
              , (bp::arg("archive")));
    }


}
