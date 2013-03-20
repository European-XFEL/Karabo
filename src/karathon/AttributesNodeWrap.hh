/* 
 * File:   AttributesNodeWrap.hh
 * Author: esenov
 *
 * Created on March 20, 2013, 2:28 PM
 */

#ifndef ATTRIBUTESNODEWRAP_HH
#define	ATTRIBUTESNODEWRAP_HH

#include <boost/python.hpp>
#ifdef KARATHON_BOOST_NUMPY
#include <boost/numpy.hpp>
namespace bn = boost::numpy;
#endif
#include <karabo/util/Hash.hh>
#include <karabo/util/FromLiteral.hh>
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karabo {
    namespace pyexfel {

        
        class AttributesNodeWrap {
        public:

            static bp::object pythonGetKey(karabo::util::Hash::Attributes::Node& node) {
                return bp::object(node.getKey());
            }

            static void pythonSetValue(karabo::util::Hash::Attributes::Node& node, const bp::object& obj) {
                boost::any any;
                Wrapper::toAny(obj, any);
                node.setValue(any);
            }

            static bp::object pythonGetValue(karabo::util::Hash::Attributes::Node& node) {
                return Wrapper::toObject(node.getValueAsAny());
            }

            static bp::object pythonGetValueAs(karabo::util::Hash::Attributes::Node& node, const std::string& type) {
                using namespace karabo::util;
                Types::ReferenceType reftype = Types::from<FromLiteral>(type);
                switch (reftype) {
                    case Types::ANY:
                        return Wrapper::toObject(node.getValueAsAny());
                    case Types::BOOL:
                        return bp::object(node.getValueAs<bool>());
                    case Types::CHAR:
                        return bp::object(node.getValueAs<char>());
                    case Types::INT8:
                        return bp::object(node.getValueAs<signed char>());
                    case Types::UINT8:
                        return bp::object(node.getValueAs<unsigned char>());
                    case Types::INT16:
                        return bp::object(node.getValueAs<short>());
                    case Types::UINT16:
                        return bp::object(node.getValueAs<unsigned short>());
                    case Types::INT32:
                        return bp::object(node.getValueAs<int>());
                    case Types::UINT32:
                        return bp::object(node.getValueAs<unsigned int>());
                    case Types::INT64:
                        return bp::object(node.getValueAs<long long>());
                    case Types::UINT64:
                        return bp::object(node.getValueAs<unsigned long long>());
                    case Types::FLOAT:
                        return bp::object(node.getValueAs<float>());
                    case Types::DOUBLE:
                        return bp::object(node.getValueAs<double>());
                    case Types::STRING:
                        return bp::object(node.getValueAs<std::string>());
                    case Types::VECTOR_BOOL:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<bool, std::vector > ());
                    case Types::VECTOR_CHAR:
                        return Wrapper::fromStdVectorToPyByteArray(node.getValueAs<char, std::vector>());
                    case Types::VECTOR_INT8:
                        return Wrapper::fromStdVectorToPyByteArray(node.getValueAs<signed char, std::vector > ());
                    case Types::VECTOR_UINT8:
                        return Wrapper::fromStdVectorToPyByteArray(node.getValueAs<unsigned char, std::vector>());
                    case Types::VECTOR_INT16:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<short, std::vector>());
                    case Types::VECTOR_UINT16:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<unsigned short, std::vector>());
                    case Types::VECTOR_INT32:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<int, std::vector>());
                    case Types::VECTOR_UINT32:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<unsigned int, std::vector>());
                    case Types::VECTOR_INT64:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<long long, std::vector>());
                    case Types::VECTOR_UINT64:
                        return Wrapper::fromStdVectorToPyArray(node.getValueAs<unsigned long long, std::vector>());
//                    case Types::HASH:
//                        return bp::object(node.getValueAs<karabo::util::Hash>());
//                    case Types::VECTOR_HASH:
//                        return Wrapper::fromStdVectorToPyList(node.getValueAs<karabo::util::Hash, std::vector>());
                    default:
                        break;
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
            }


            static bp::object pythonGetType(karabo::util::Hash::Attributes::Node& node) {
                using namespace karabo::util;
                return bp::str(Types::to<ToLiteral>(node.getType()));
            }
            
            static void pythonSetType(karabo::util::Hash::Attributes::Node& node, const std::string& type) {
                using namespace karabo::util;
                node.setType(Types::from<FromLiteral>(type));
            }
        };        
    }
}

#endif	/* ATTRIBUTESNODEWRAP_HH */

