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
 * File:   AttributesWrap.hh
 * Author: esenov
 *
 * Created on March 20, 2013, 10:15 AM
 */

#ifndef ATTRIBUTESWRAP_HH
#define ATTRIBUTESWRAP_HH

#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/Types.hh>


namespace karathon {

    class AttributesWrap {
        struct null_deleter {
            void operator()(void const*) const {}
        };

       public:
        static bool has(karabo::util::Hash::Attributes& self, const std::string& key) {
            return self.has(key);
        }

        static bool is(karabo::util::Hash::Attributes& self, const std::string& key, const std::string& type) {
            using namespace karabo::util;
            Types::ReferenceType reftype = Types::from<FromLiteral>(type);
            return self.is(key, reftype);
        }

        static void erase(karabo::util::Hash::Attributes& self, const std::string& key) {
            self.erase(key);
        }

        static bp::object size(karabo::util::Hash::Attributes& self) {
            return bp::object(self.size());
        }

        static bool empty(karabo::util::Hash::Attributes& self) {
            return self.empty();
        }

        static void clear(karabo::util::Hash::Attributes& self) {
            self.clear();
        }

        static bp::object getNode(karabo::util::Hash::Attributes& self, const std::string& key) {
            using namespace karabo::util;
            Hash::Attributes::Node& nodeRef = self.getNode(key);
            boost::optional<Hash::Attributes::Node&> node(nodeRef);
            return bp::object(boost::shared_ptr<Hash::Attributes::Node>(&(*node), null_deleter()));
        }

        static bp::object get(karabo::util::Hash::Attributes& self, const std::string& key) {
            return Wrapper::toObject(self.getAny(key));
        }

        static bp::object getAs(karabo::util::Hash::Attributes& self, const std::string& key,
                                const bp::object& o_type) {
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
                    return bp::object(self.getAs<bool>(key));
                case Types::CHAR:
                    return bp::object(self.getAs<char>(key));
                case Types::INT8:
                    return bp::object(self.getAs<signed char>(key));
                case Types::UINT8:
                    return bp::object(self.getAs<unsigned char>(key));
                case Types::INT16:
                    return bp::object(self.getAs<short>(key));
                case Types::UINT16:
                    return bp::object(self.getAs<unsigned short>(key));
                case Types::INT32:
                    return bp::object(self.getAs<int>(key));
                case Types::UINT32:
                    return bp::object(self.getAs<unsigned int>(key));
                case Types::INT64:
                    return bp::object(self.getAs<long long>(key));
                case Types::UINT64:
                    return bp::object(self.getAs<unsigned long long>(key));
                case Types::FLOAT:
                    return bp::object(self.getAs<float>(key));
                case Types::DOUBLE:
                    return bp::object(self.getAs<double>(key));
                case Types::STRING:
                    return bp::object(self.getAs<std::string>(key));
                case Types::VECTOR_BOOL:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<bool, std::vector>(key));
                case Types::VECTOR_CHAR:
                    return Wrapper::fromStdVectorToPyByteArray(self.getAs<char, std::vector>(key));
                case Types::VECTOR_INT8:
                    return Wrapper::fromStdVectorToPyByteArray(self.getAs<signed char, std::vector>(key));
                case Types::VECTOR_UINT8:
                    return Wrapper::fromStdVectorToPyByteArray(self.getAs<unsigned char, std::vector>(key));
                case Types::VECTOR_INT16:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<short, std::vector>(key));
                case Types::VECTOR_UINT16:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned short, std::vector>(key));
                case Types::VECTOR_INT32:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<int, std::vector>(key));
                case Types::VECTOR_UINT32:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned int, std::vector>(key));
                case Types::VECTOR_INT64:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<long long, std::vector>(key));
                case Types::VECTOR_UINT64:
                    return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned long long, std::vector>(key));
                    //                    case Types::HASH:
                    //                        return bp::object(self.getAs<karabo::util::Hash>(key));
                    //                    case Types::VECTOR_HASH:
                    //                        return Wrapper::fromStdVectorToPyList(self.getAs<karabo::util::Hash,
                    //                        std::vector>(key));
                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
        }

        static bp::object set(karabo::util::Hash::Attributes& self, const std::string& key, const bp::object& value) {
            boost::any any;
            Wrapper::toAny(value, any);
            return bp::object(self.set(key, any));
        }

        //            static bp::object find(karabo::util::Hash::Attributes& self, const std::string& key) {
        //                karabo::util::Hash::Attributes::const_map_iterator it = self.find(key);
        //                if (it == self.mend())
        //                    return bp::object();
        //                return bp::object(it);
        //            }
        //
        //            static bp::object getIt(karabo::util::Hash::Attributes& self, const bp::object& obj) {
        //                if (bp::extract<karabo::util::Hash::Attributes::const_map_iterator>(obj).check()) {
        //                    karabo::util::Hash::Attributes::const_map_iterator it =
        //                    bp::extract<karabo::util::Hash::Attributes::const_map_iterator>(obj); boost::any any =
        //                    self.get(it); std::cout << "getIt for const_map_iterator:" << any.type().name() <<
        //                    std::endl; return Wrapper::toObject(any);
        //                } else if (bp::extract<karabo::util::Hash::Attributes::map_iterator>(obj).check()) {
        //                    std::cout << "getIt for map_iterator:" << std::endl;
        //                    karabo::util::Hash::Attributes::map_iterator it =
        //                    bp::extract<karabo::util::Hash::Attributes::map_iterator>(obj); return
        //                    Wrapper::toObject(self.get<boost::any>(it));
        //                }
        //                throw KARABO_NOT_SUPPORTED_EXCEPTION("Python type cannot be converted to
        //                Hash::Attributes::map_iterator");
        //            }

        static bp::object __getitem__(karabo::util::Hash::Attributes& self, const bp::object& obj) {
            if (bp::extract<karabo::util::Hash::Attributes::Node&>(obj).check()) {
                karabo::util::Hash::Attributes::Node& node = bp::extract<karabo::util::Hash::Attributes::Node&>(obj);
                if (node.getType() == karabo::util::Types::HASH) {
                    boost::shared_ptr<karabo::util::Hash> hash =
                          boost::shared_ptr<karabo::util::Hash>(&node.getValue<karabo::util::Hash>(), null_deleter());
                    return bp::object(hash);
                }
                return Wrapper::toObject(node.getValueAsAny(), false);
            } else if (bp::extract<std::string>(obj).check()) {
                karabo::util::Hash::Attributes::Node& node = self.getNode(bp::extract<std::string>(obj));
                if (node.getType() == karabo::util::Types::HASH) {
                    boost::shared_ptr<karabo::util::Hash> hash =
                          boost::shared_ptr<karabo::util::Hash>(&node.getValue<karabo::util::Hash>(), null_deleter());
                    return bp::object(hash);
                }
                return Wrapper::toObject(node.getValueAsAny(), false);
            }
            throw KARABO_PYTHON_EXCEPTION("Invalid type for Hash index. The type should be 'Node' or 'str'!");
        }
    };
} // namespace karathon

#endif /* ATTRIBUTESWRAP_HH */
