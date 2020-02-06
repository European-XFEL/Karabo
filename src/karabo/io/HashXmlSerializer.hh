/* 
 * File:   HashXmlSerializer.hh
 * Author: <burkhard.heisen@xsmail.com>
 * 
 * Created on February 21, 2013, 8:42 AM
 *
 */

#ifndef HASHXMLSERIALIZER_H
#define	HASHXMLSERIALIZER_H

#include <pugixml.hpp>
#include <karabo/util/Hash.hh>

#include "TextSerializer.hh"

namespace karabo {

    namespace io {

        /**
         * @class HashXmlSerializer
         * @brief The HashXmlSerializer provides an implementation of TextSerializer
         *        for the karabo::util::Hash
         * 
         * While a karabo::util::Hash can in principle hold arbitrary data types, Hash
         * serialization is limited to data types known to the karabo::util::Types type
         * system. Hashes containing other data types will lead to exceptions during
         * serialization.
         */
        class HashXmlSerializer : public TextSerializer<karabo::util::Hash> {


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

            static void expectedParameters(karabo::util::Schema& expected);

            HashXmlSerializer(const karabo::util::Hash& hash);

            void save(const karabo::util::Hash& object, std::string& archive);

            void load(karabo::util::Hash& object, const std::string& archive);

            void load(karabo::util::Hash& object, const char* archive);

            void save(const std::vector<karabo::util::Hash>& objects, std::string& archive);

            void load(std::vector<karabo::util::Hash>& objects, const std::string& archive);

            virtual ~HashXmlSerializer() {
            };

        private:

            void createXml(const karabo::util::Hash& object, pugi::xml_node& node) const;

            void createHash(karabo::util::Hash& object, pugi::xml_node node) const;

            void writeAttributes(const karabo::util::Hash::Attributes& attrs, pugi::xml_node& node) const;

            /**
             * Reads all the hash attributes that are convertible from string from a given xml node.
             * @param attrs the set of attrs that are convertible from string.
             * @param node the xml node with the attributes to be read.
             * @return true if all the attributes in the xml node have been read; false if there is at least
             * one attribute that is not convertible from string that should still be processed.
             */
            bool readStrConvertibleAttrs(karabo::util::Hash::Attributes& attrs, const pugi::xml_node& node) const;

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
            void extractNonStrConvertibleAttrs(std::vector<karabo::util::Hash>& nonStrAttrs, const pugi::xml_node& node) const;

            /**
             * Adds hash attributes non convertible from string to a given hash on a given path. The hash attributes
             * are stored in a vector of hashes argument.
             * @param hash the hash that will have the attributes added to.
             * @param hashPath the path in the hash where the attributes will be added.
             * @param attrs the vector with the attributes to be added. Each attribute is an element of this vector and
             * has a single key that corresponds to the attribute name and whose value is the attribute value.
             */
            void addNonStrConvertibleAttrs(karabo::util::Hash& hash,
                                       const std::string& hashPath,
                                       const std::vector<karabo::util::Hash>& attrs) const;

            std::pair<std::string, karabo::util::Types::ReferenceType> readXmlAttribute(const std::string& xmlAttribute) const;

            struct CustomWriter : public pugi::xml_writer {


                std::string& _result;

                CustomWriter(std::string & archive) : _result(archive) {
                }

                void write(const void* data, const size_t nBytes) {
                    _result += std::string(static_cast<const char*> (data), nBytes);
                }
            };

            std::string escapeElementName(const std::string& data) const;
            std::string unescapeElementName(const std::string& data) const;

        };
    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::TextSerializer<karabo::util::Hash>)

#endif	

