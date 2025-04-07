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
 * File:   HashXmlSerializer.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#ifndef KARABO_DATA_IO_HASHXMLSERIALIZER_HH
#define KARABO_DATA_IO_HASHXMLSERIALIZER_HH

#include <pugixml.hpp>

#include "TextSerializer.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {

    namespace data {

        /**
         * @class HashXmlSerializer
         * @brief The HashXmlSerializer provides an implementation of TextSerializer
         *        for the karabo::data::Hash
         *
         * While a karabo::data::Hash can in principle hold arbitrary data types, Hash
         * serialization is limited to data types known to the karabo::data::Types type
         * system. Hashes containing other data types will lead to exceptions during
         * serialization.
         */
        class HashXmlSerializer : public TextSerializer<karabo::data::Hash> {
            std::string m_indentation;
            std::string m_xmlns;
            std::string m_prefix;
            bool m_writeDataTypes;
            bool m_readDataTypes;
            bool m_insertXmlNamespace;

            // Helper variables
            std::string m_typeFlag;
            std::string m_artificialRootFlag;
            std::string m_itemFlag;
            bool m_writeCompact;

           public:
            KARABO_CLASSINFO(HashXmlSerializer, "Xml", "1.0")

            static void expectedParameters(karabo::data::Schema& expected);

            HashXmlSerializer(const karabo::data::Hash& hash);

            void save(const karabo::data::Hash& object, std::string& archive);

            void load(karabo::data::Hash& object, const std::string& archive);

            void load(karabo::data::Hash& object, const char* archive);

            void save(const std::vector<karabo::data::Hash>& objects, std::string& archive);

            void load(std::vector<karabo::data::Hash>& objects, const std::string& archive);

            virtual ~HashXmlSerializer(){};

           private:
            void createXml(const karabo::data::Hash& object, pugi::xml_node& node) const;

            void createHash(karabo::data::Hash& object, pugi::xml_node node) const;

            void writeAttributes(const karabo::data::Hash::Attributes& attrs, pugi::xml_node& node) const;

            /**
             * Reads all the hash attributes that are convertible from string from a given xml node.
             * @param attrs the set of attrs that are convertible from string.
             * @param node the xml node with the attributes to be read.
             * @return true if all the attributes in the xml node have been read; false if there is at least
             * one attribute that is not convertible from string that should still be processed.
             */
            bool readStrConvertibleAttrs(karabo::data::Hash::Attributes& attrs, const pugi::xml_node& node) const;

            /**
             * Extracts all the hash attributes that are non convertible from string from a given xml node.
             * @param node the xml node with the attributes to be read.
             * @param nonStrAttrs vector of hashes with each hash corresponding to a nonStringfiedAttr read from the
             * xml node. Each hash in the vector has one node with the attribute name for its key and the attribute
             * value for its value.
             *
             * @note: currently there are two types of attributes that are not convertible from string: VECTOR_HASH
             * and SCHEMA.
             */
            void extractNonStrConvertibleAttrs(std::vector<karabo::data::Hash>& nonStrAttrs,
                                               const pugi::xml_node& node) const;

            /**
             * Adds hash attributes non convertible from string to a given hash on a given path. The hash attributes
             * are stored in a vector of hashes argument.
             * @param hash the hash that will have the attributes added to.
             * @param hashPath the path in the hash where the attributes will be added.
             * @param attrs the vector with the attributes to be added. Each attribute is an element of this vector and
             *              has a single key that corresponds to the attribute name and whose value is the attribute
             * value. This value will be 'moved away' to the attributes to avoid copies.
             */
            void addNonStrConvertibleAttrs(karabo::data::Hash& hash, const std::string& hashPath,
                                           std::vector<karabo::data::Hash>& attrs) const;

            std::pair<std::string, karabo::data::Types::ReferenceType> readXmlAttribute(
                  const std::string& xmlAttribute) const;

            struct CustomWriter : public pugi::xml_writer {
                std::string& _result;

                CustomWriter(std::string& archive) : _result(archive) {}

                void write(const void* data, const size_t nBytes) {
                    _result += std::string(static_cast<const char*>(data), nBytes);
                }
            };

            std::string escapeElementName(const std::string& data) const;
            std::string unescapeElementName(const std::string& data) const;
        };
    } // namespace data
} // namespace karabo

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::data::TextSerializer<karabo::data::Hash>)

#endif
