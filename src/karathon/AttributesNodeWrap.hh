/* 
 * File:   AttributesNodeWrap.hh
 * Author: esenov
 *
 * Created on March 20, 2013, 2:28 PM
 */

#ifndef ATTRIBUTESNODEWRAP_HH
#define	ATTRIBUTESNODEWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/FromLiteral.hh>
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {


    class AttributesNodeWrap {


        struct null_deleter {

            void operator()(void const *) const {
            }
        };
    public:

        typedef boost::shared_ptr<karabo::util::Hash::Attributes::Node> Pointer;

        static bp::object getKey(const Pointer& node) {
            return bp::object(node->getKey());
        }

        static void setValue(const Pointer& node, const bp::object& obj) {
            boost::any any;
            Wrapper::toAny(obj, any);
            node->setValue(any);
        }

        static bp::object getValue(const Pointer& node) {
            using namespace karabo::util;
            boost::any& a = node->getValueAsAny();
            // handle Hash differently returning reference to Hash
            if (a.type() == typeid (Hash)) {
                Hash& hash = boost::any_cast<Hash&>(a);
                boost::shared_ptr<Hash> p(&hash, null_deleter());
                return bp::object(p);
            }
            return Wrapper::toObject(a);
        }

        static bp::object getValueAs(const Pointer& node, const PyTypes::ReferenceType& reftype) {
            using namespace karabo::util;
            switch (reftype) {
                case PyTypes::BOOL:
                    return bp::object(node->getValueAs<bool>());
                case PyTypes::CHAR:
                    return bp::object(node->getValueAs<char>());
                case PyTypes::INT8:
                    return bp::object(node->getValueAs<signed char>());
                case PyTypes::UINT8:
                    return bp::object(node->getValueAs<unsigned char>());
                case PyTypes::INT16:
                    return bp::object(node->getValueAs<short>());
                case PyTypes::UINT16:
                    return bp::object(node->getValueAs<unsigned short>());
                case PyTypes::INT32:
                    return bp::object(node->getValueAs<int>());
                case PyTypes::UINT32:
                    return bp::object(node->getValueAs<unsigned int>());
                case PyTypes::INT64:
                    return bp::object(node->getValueAs<long long>());
                case PyTypes::UINT64:
                    return bp::object(node->getValueAs<unsigned long long>());
                case PyTypes::FLOAT:
                    return bp::object(node->getValueAs<float>());
                case PyTypes::DOUBLE:
                    return bp::object(node->getValueAs<double>());
                case PyTypes::STRING:
                    return bp::object(node->getValueAs<std::string>());
                case PyTypes::VECTOR_BOOL:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<bool, std::vector > ());
                case PyTypes::VECTOR_CHAR:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<char, std::vector>());
                case PyTypes::VECTOR_INT8:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<signed char, std::vector > ());
                case PyTypes::VECTOR_UINT8:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<unsigned char, std::vector>());
                case PyTypes::VECTOR_INT16:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<short, std::vector>());
                case PyTypes::VECTOR_UINT16:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned short, std::vector>());
                case PyTypes::VECTOR_INT32:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<int, std::vector>());
                case PyTypes::VECTOR_UINT32:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned int, std::vector>());
                case PyTypes::VECTOR_INT64:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<long long, std::vector>());
                case PyTypes::VECTOR_UINT64:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned long long, std::vector>());
                    //                    case Types::HASH:
                    //                        return bp::object(node->getValueAs<karabo::util::Hash>());
                    //                    case Types::VECTOR_HASH:
                    //                        return Wrapper::fromStdVectorToPyList(node->getValueAs<karabo::util::Hash, std::vector>());
                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
        }

        static bp::object getType(const Pointer& node) {
            using namespace karabo::util;
            return bp::str(Types::to<ToLiteral>(node->getType()));
        }

        static void setType(const Pointer& node, const bp::object& o_type) {
            using namespace karabo::util;
            if (bp::extract<std::string>(o_type).check()) {
                std::string type = bp::extract<std::string>(o_type);
                node->setType(Types::from<FromLiteral>(type));
            } else if (bp::extract<PyTypes::ReferenceType>(o_type).check()) {
                PyTypes::ReferenceType type = bp::extract<PyTypes::ReferenceType>(o_type);
                node->setType(PyTypes::to(type));
            }
        }
    };
}

#endif	/* ATTRIBUTESNODEWRAP_HH */

