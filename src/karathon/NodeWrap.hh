/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   ElementWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on March 17, 2013, 6:29 PM
 */

#ifndef ELEMENTWRAP_HH
#define ELEMENTWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>

#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {

    class NodeWrap {
        struct null_deleter {
            void operator()(void const*) const {}
        };

       public:
        typedef boost::shared_ptr<karabo::util::Hash::Node> Pointer;

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
            if (a.type() == typeid(Hash)) {
                Hash& hash = boost::any_cast<Hash&>(a);
                boost::shared_ptr<Hash> p(&hash, null_deleter());
                return bp::object(p);
            }
            return Wrapper::toObject(a);
        }

        static bp::object getValueAs(const Pointer& node, const bp::object& o_type) {
            using namespace karabo::util;
            Types::ReferenceType reftype = Types::UNKNOWN;
            if (bp::extract<std::string>(o_type).check()) {
                std::string type = bp::extract<std::string>(o_type);
                reftype = Types::from<FromLiteral>(type);
            } else if (bp::extract<PyTypes::ReferenceType>(o_type).check()) {
                PyTypes::ReferenceType type = bp::extract<PyTypes::ReferenceType>(o_type);
                reftype = PyTypes::to(type);
            }
            switch (reftype) {
                case Types::BOOL:
                    return bp::object(node->getValueAs<bool>());
                case Types::CHAR:
                    return bp::object(node->getValueAs<char>());
                case Types::INT8:
                    return bp::object(node->getValueAs<signed char>());
                case Types::UINT8:
                    return bp::object(node->getValueAs<unsigned char>());
                case Types::INT16:
                    return bp::object(node->getValueAs<short>());
                case Types::UINT16:
                    return bp::object(node->getValueAs<unsigned short>());
                case Types::INT32:
                    return bp::object(node->getValueAs<int>());
                case Types::UINT32:
                    return bp::object(node->getValueAs<unsigned int>());
                case Types::INT64:
                    return bp::object(node->getValueAs<long long>());
                case Types::UINT64:
                    return bp::object(node->getValueAs<unsigned long long>());
                case Types::FLOAT:
                    return bp::object(node->getValueAs<float>());
                case Types::DOUBLE:
                    return bp::object(node->getValueAs<double>());
                case Types::STRING:
                    return bp::object(node->getValueAs<std::string>());
                case Types::VECTOR_BOOL:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<bool, std::vector>());
                case Types::VECTOR_CHAR:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<char, std::vector>());
                case Types::VECTOR_INT8:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<signed char, std::vector>());
                case Types::VECTOR_UINT8:
                    return Wrapper::fromStdVectorToPyByteArray(node->getValueAs<unsigned char, std::vector>());
                case Types::VECTOR_INT16:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<short, std::vector>());
                case Types::VECTOR_UINT16:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned short, std::vector>());
                case Types::VECTOR_INT32:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<int, std::vector>());
                case Types::VECTOR_UINT32:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned int, std::vector>());
                case Types::VECTOR_INT64:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<long long, std::vector>());
                case Types::VECTOR_UINT64:
                    return Wrapper::fromStdVectorToPyArray(node->getValueAs<unsigned long long, std::vector>());
                    //                    case Types::HASH:
                    //                        return bp::object(node->getValueAs<karabo::util::Hash>());
                    //                    case Types::VECTOR_HASH:
                    //                        return Wrapper::fromStdVectorToPyList(node->getValueAs<karabo::util::Hash,
                    //                        std::vector>());

                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
        }

        static void setAttribute(const Pointer& node, const std::string& key, const bp::object& obj) {
            boost::any value;
            Wrapper::toAny(obj, value);
            node->setAttribute(key, value);
        }

        static bp::object getAttribute(const Pointer& node, const std::string& key) {
            return Wrapper::toObject(node->getAttributeAsAny(key));
        }

        static bp::object getAttributeAs(const Pointer& node, const std::string& key, const bp::object& o_type) {
            using namespace karabo::util;
            Types::ReferenceType reftype = Types::UNKNOWN;
            if (bp::extract<std::string>(o_type).check()) {
                std::string type = bp::extract<std::string>(o_type);
                reftype = Types::from<FromLiteral>(type);
            } else if (bp::extract<PyTypes::ReferenceType>(o_type).check()) {
                PyTypes::ReferenceType type = bp::extract<PyTypes::ReferenceType>(o_type);
                reftype = PyTypes::to(type);
            }

            switch (reftype) {
                case Types::BOOL:
                    return bp::object(node->getAttributeAs<bool>(key));
                case Types::CHAR:
                    return bp::object(node->getAttributeAs<char>(key));
                case Types::INT8:
                    return bp::object(node->getAttributeAs<signed char>(key));
                case Types::UINT8:
                    return bp::object(node->getAttributeAs<unsigned char>(key));
                case Types::INT16:
                    return bp::object(node->getAttributeAs<short>(key));
                case Types::UINT16:
                    return bp::object(node->getAttributeAs<unsigned short>(key));
                case Types::INT32:
                    return bp::object(node->getAttributeAs<int>(key));
                case Types::UINT32:
                    return bp::object(node->getAttributeAs<unsigned int>(key));
                case Types::INT64:
                    return bp::object(node->getAttributeAs<long long>(key));
                case Types::UINT64:
                    return bp::object(node->getAttributeAs<unsigned long long>(key));
                case Types::FLOAT:
                    return bp::object(node->getAttributeAs<float>(key));
                case Types::DOUBLE:
                    return bp::object(node->getAttributeAs<double>(key));
                case Types::STRING:
                    return bp::object(node->getAttributeAs<std::string>(key));
                    //                    case Types::VECTOR_BOOL:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<bool,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_CHAR:
                    //                        return Wrapper::fromStdVectorToPyByteArray(node->getAttributeAs<char,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_INT8:
                    //                        return Wrapper::fromStdVectorToPyByteArray(node->getAttributeAs <signed
                    //                        char, std::vector>(key));
                    //                    case Types::VECTOR_UINT8:
                    //                        return Wrapper::fromStdVectorToPyByteArray(node->getAttributeAs<unsigned
                    //                        char, std::vector>(key));
                    //                    case Types::VECTOR_INT16:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<short,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_UINT16:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<unsigned
                    //                        short, std::vector>(key));
                    //                    case Types::VECTOR_INT32:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<int,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_UINT32:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<unsigned int,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_INT64:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<long long,
                    //                        std::vector>(key));
                    //                    case Types::VECTOR_UINT64:
                    //                        return Wrapper::fromStdVectorToPyArray(node->getAttributeAs<unsigned long
                    //                        long, std::vector>(key));
                    //                    case Types::HASH:
                    //                        return bp::object(node->getAttributeAs<karabo::util::Hash>(key));
                    //                    case Types::VECTOR_HASH:
                    //                        return
                    //                        Wrapper::fromStdVectorToPyList(node->getAttributeAs<karabo::util::Hash,
                    //                        std::vector>(key));
                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
        }

        static bool hasAttribute(const Pointer& node, const std::string& key) {
            return node->hasAttribute(key);
        }

        static void setAttributes(const Pointer& node, const bp::object& obj) {
            if (bp::extract<karabo::util::Hash::Attributes>(obj).check())
                node->setAttributes(bp::extract<karabo::util::Hash::Attributes>(obj));
            else throw KARABO_PYTHON_EXCEPTION("Python object is not of a Hash.Attributes type");
        }

        static const karabo::util::Hash::Attributes& getAttributes(const Pointer& node) {
            return static_cast<const karabo::util::Hash::Attributes&>(node->getAttributes());
        }

        static bp::object copyAttributes(const Pointer& node) {
            return bp::object(node->getAttributes());
        }

        static bp::object getType(const Pointer& node) {
            return bp::object(PyTypes::from(node->getType()));
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
} // namespace karathon

#endif /* ELEMENTWRAP_HH */
