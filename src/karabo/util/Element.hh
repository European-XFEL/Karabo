/* 
 * File:   Element.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on December 14, 2012, 11:07 AM
 */

#ifndef KARABO_UTIL_NODE_HH
#define	KARABO_UTIL_NODE_HH

#include <typeinfo>
#include <string>
#include <complex>
#include <vector>

#include <boost/any.hpp>
#include <boost/cast.hpp>

#include "Types.hh"
#include "Exception.hh"
#include "FromTypeInfo.hh"

namespace karabo {
    namespace util {

        // Class forward (needed to prevent assignment of Hash to Attribute)
        class Hash;
        template <class T> class GenericElement;

        template<typename KeyType, typename AttributesType = bool>
        class Element {

            // Grant friendship to container
            template<typename T, typename U>
            friend class OrderedMap;
            
            // Grant friendship to GenericElement (needs to setKey)
            template<class T>
            friend class GenericElement;

            // Members
            KeyType m_key;
            AttributesType m_attributes;
            boost::any m_value;

        public:

            Element();

            Element(const KeyType& key, boost::any value);

            template <class ValueType>
            Element(const KeyType& key, const ValueType& value);

            // TODO Constructor with attributes container ??

            virtual ~Element();

            const KeyType& getKey() const;

            template<class ValueType>
            inline void setValue(const ValueType& value);

            void setValue(const char* const& value);

            void setValue(const wchar_t* const& value);

            void setValue(wchar_t* const& value);

            void setValue(const boost::any& value);

            void setValue(const Element<KeyType, AttributesType>& value);

            //        void swap(const Node<KeyType, AttributeType>& other);

            template<class ValueType>
            inline const ValueType& getValue() const;

            template<class ValueType>
            inline ValueType& getValue();

            boost::any& getValueAsAny();

            const boost::any& getValueAsAny() const;

            template <typename ValueType >
            ValueType getValueAs() const;

            template<typename T,
            template <typename Elem, typename = std::allocator<Elem> > class Cont >
            Cont<T> getValueAs() const;

            template<class T>
            inline void setAttribute(const std::string& key, const T& value);

            inline void setAttribute(const std::string& key, const boost::any& value);

            inline void setAttribute(const std::string& key, const Hash& value);

            inline void setAttribute(const std::string& key, const std::vector<Hash>& value);

            template<class T>
            inline T& getAttribute(const std::string& key);

            template<class T>
            inline void getAttribute(const std::string& key, T& value) const;

            template<class T>
            inline const T& getAttribute(const std::string& key) const;

            template<class T>
            inline void getAttribute(const std::string& key, const T& value) const;

            inline const boost::any& getAttributeAsAny(const std::string& key) const;

            inline boost::any& getAttributeAsAny(const std::string& key);

            template <class T>
            inline T getAttributeAs(const std::string& key) const;
            
            template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
            inline Cont<T> getAttributeAs(const std::string& key) const;
            
            Element<KeyType>& getAttributeNode(const std::string& key);
            
            const Element<KeyType>& getAttributeNode(const std::string& key) const;

            inline bool hasAttribute(const std::string& key) const;

            inline void setAttributes(const AttributesType& attributes);

            inline const AttributesType& getAttributes() const;

            inline AttributesType& getAttributes();

            template <typename T>
            inline bool is() const;

            Types::ReferenceType getType() const;

            void setType(const Types::ReferenceType& tgtType);

            bool operator==(const Element<KeyType, AttributesType>& other) const;

            bool operator!=(const Element<KeyType, AttributesType>& other) const;

        private:

            inline void setKey(const KeyType& key);

            inline std::string getValueAsString() const;

        };

        /**********************************************************************
         *
         * Implementation Node
         * 
         **********************************************************************/

        // TODO This should be implemented in a cleaner way

        template<class KeyType, typename AttributeType>
        Element<KeyType, AttributeType>::Element() {
            //            if (typeid (m_attributes) == typeid (bool)) {
            //                *reinterpret_cast<bool*> (&m_attributes) = false;
            //            }
        }

        template<class KeyType, typename AttributeType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, boost::any value) : m_key(key), m_value(value) {
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, const ValueType& value) : m_key(key) {
            this->setValue(value);
        }

        template<class KeyType, typename AttributesType>
        Element<KeyType, AttributesType>::~Element() {
        }

        template<class KeyType, typename AttributeType>
        inline const KeyType & Element<KeyType, AttributeType>::getKey() const {
            return m_key;
        }

        template<class KeyType, typename AttributeType>
        void Element<KeyType, AttributeType>::setKey(const KeyType& key) {
            m_key = key;
        }

        template<class KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::operator==(const Element<KeyType, AttributeType>& other) const {
            return this->m_key == other.m_key;
        }

        template<class KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::operator!=(const Element<KeyType, AttributeType>& other) const {
            return this->m_key != other.m_key;
        }

        template<class KeyType, typename AttributeType>
        template <typename T>
        inline bool Element<KeyType, AttributeType>::is() const {
            return m_value.type() == typeid (T);
        }

        template<class KeyType, typename AttributeType>
        Types::ReferenceType Element<KeyType, AttributeType>::getType() const {
            return FromType<FromTypeInfo>::from(m_value.type());
        }

        template<class KeyType, typename AttributeType>
        template<class ValueType>
        inline void Element<KeyType, AttributeType>::setValue(const ValueType& value) {
            m_value = value;
        }

        template<class KeyType, class ValueType>
        inline void Element<KeyType, ValueType>::setValue(const char* const& value) {
            m_value = std::string(value);
        }

        template<class KeyType, class ValueType>
        inline void Element<KeyType, ValueType>::setValue(const wchar_t* const& value) {
            m_value = std::wstring(value);
        }

        template<class KeyType, class ValueType>
        inline void Element<KeyType, ValueType>::setValue(wchar_t* const& value) {
            m_value = std::wstring(value);
        }

        template<class KeyType, class ValueType>
        inline void Element<KeyType, ValueType>::setValue(const boost::any & value) {
            m_value = value;
        }

        template<class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const Element<KeyType, AttributeType>& other) {
            if (this != &other) {
                this->m_value = other.m_value;
            }
        }

        template<class KeyType, typename AttributeType>
        template<class ValueType>
        inline const ValueType& Element<KeyType, AttributeType>::getValue() const {
            const ValueType* ptr = boost::any_cast<ValueType > (&m_value);
            if (ptr) return *ptr;
            else throw KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, m_value.type(), typeid (ValueType)));
        }

        template<class KeyType, typename AttributeType>
        template<class ValueType>
        inline ValueType& Element<KeyType, AttributeType>::getValue() {
            ValueType* ptr = boost::any_cast<ValueType > (&m_value);
            if (ptr) return *ptr;
            else throw KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, m_value.type(), typeid (ValueType)));
        }

        template<class KeyType, typename AttributeType>
        template <typename ValueType >
        inline ValueType Element<KeyType, AttributeType>::getValueAs() const {
            if (m_value.type() == typeid (ValueType)) return this->getValue<ValueType > ();

            Types::ReferenceType srcType = this->getType();
            Types::ReferenceType tgtType = Types::from<ValueType > ();

            if (srcType == Types::UNKNOWN) throw KARABO_CAST_EXCEPTION("Unknown source type for key: \"" + m_key + "\". Cowardly refusing to cast.");

            try {
                return ValueType(karabo::util::fromString<ValueType > (this->getValueAsString()));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, srcType, tgtType)));
            }
            return ValueType();
        }

        template<class KeyType, typename AttributeType>
        template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
        inline Cont<T> Element<KeyType, AttributeType>::getValueAs() const {

            Types::ReferenceType srcType = this->getType();
            Types::ReferenceType tgtType = Types::from<Cont<T> >();

            if (tgtType == srcType) return this->getValue<Cont<T> > ();
            if (srcType == Types::UNKNOWN) throw KARABO_CAST_EXCEPTION("Unknown source type for key: \"" + m_key + "\". Cowardly refusing to cast.");

            try {
                std::string value = this->getValueAsString();
                
                if (value.empty()) return Cont<T>();
                
                return karabo::util::fromString<T, Cont > (value);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, srcType, tgtType)));
                return Cont<T>(); // Make the compiler happy
            }
        }

        template<class KeyType, typename AttributeType>
        boost::any& Element<KeyType, AttributeType>::getValueAsAny() {
            return m_value;
        }

        template<class KeyType, typename AttributeType>
        const boost::any& Element<KeyType, AttributeType>::getValueAsAny() const {
            return m_value;
        }

        template<class KeyType, typename AttributeType>
        inline const AttributeType&
        Element<KeyType, AttributeType>::getAttributes() const {
            return m_attributes;
        }

        template<class KeyType, typename AttributeType>
        inline AttributeType&
        Element<KeyType, AttributeType>::getAttributes() {
            return m_attributes;
        }

        template<class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttributes(const AttributeType& attributes) {
            m_attributes = attributes;
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, const T& value) {
            m_attributes.template set<T>(key, value);
        }

        template<typename KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, const boost::any& value) {
            m_attributes.set(key, value);
        }

        template<typename KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, const Hash& value) {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Attribute can not be of type Hash");
        }

        template<typename KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, const std::vector<Hash>& value) {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Attribute can not be of type Hash");
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline T& Element<KeyType, AttributeType>::getAttribute(const std::string& key) {
            return m_attributes.template get<T > (key);
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline void Element<KeyType, AttributeType>::getAttribute(const std::string& key, T& value) const {
            m_attributes.template get(key, value);
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline const T& Element<KeyType, AttributeType>::getAttribute(const std::string& key) const {
            return m_attributes.template get<T > (key);
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline void Element<KeyType, AttributeType>::getAttribute(const std::string& key, const T& value) const {
            m_attributes.template get(key, value);
        }

        template<typename KeyType, typename AttributeType>
        inline const boost::any& Element<KeyType, AttributeType>::getAttributeAsAny(const std::string& key) const {
            return m_attributes.getAny(key);
        }

        template<typename KeyType, typename AttributeType>
        inline boost::any& Element<KeyType, AttributeType>::getAttributeAsAny(const std::string& key) {
            return m_attributes.getAny(key);
        }

        template<typename KeyType, typename AttributeType>
        template<class T>
        inline T Element<KeyType, AttributeType>::getAttributeAs(const std::string& key) const {
            return m_attributes.template getAs<T>(key);
        }

        template<typename KeyType, typename AttributeType>
        template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
        inline Cont<T> Element<KeyType, AttributeType>::getAttributeAs(const std::string& key) const {
            return m_attributes.template getAs<T, Cont >(key);
        }
        
        template<typename KeyType, typename AttributeType>
        inline Element<KeyType>& Element<KeyType, AttributeType>::getAttributeNode(const std::string& key) {
            return m_attributes.getNode(key);
        }
        
        template<typename KeyType, typename AttributeType>
        inline const Element<KeyType>& Element<KeyType, AttributeType>::getAttributeNode(const std::string& key) const {
            return m_attributes.getNode(key);
        }

        template<typename KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::hasAttribute(const std::string& key) const {
            return m_attributes.has(key);
        }

        template<typename KeyType, typename AttributeType>
        void Element<KeyType, AttributeType>::setType(const Types::ReferenceType & tgtType) {

            #define _KARABO_HELPER_MACRO(RefType, T)\
                   case Types::RefType: m_value = this->getValueAs<T>(); break;\
                   case Types::VECTOR_##RefType: m_value = this->getValueAs<T, std::vector>(); break;

            Types::ReferenceType srcType = this->getType();
            if (tgtType == srcType) return;
            try {
                switch (tgtType) {
                        _KARABO_HELPER_MACRO(BOOL, bool)
                        _KARABO_HELPER_MACRO(CHAR, char)
                        _KARABO_HELPER_MACRO(INT8, signed char)
                        _KARABO_HELPER_MACRO(UINT8, unsigned char)
                        _KARABO_HELPER_MACRO(INT16, short)
                        _KARABO_HELPER_MACRO(UINT16, unsigned short)
                        _KARABO_HELPER_MACRO(INT32, int)
                        _KARABO_HELPER_MACRO(UINT32, unsigned int)
                        _KARABO_HELPER_MACRO(INT64, long long)
                        _KARABO_HELPER_MACRO(UINT64, unsigned long long)
                        _KARABO_HELPER_MACRO(FLOAT, float)
                        _KARABO_HELPER_MACRO(DOUBLE, double)
                        _KARABO_HELPER_MACRO(COMPLEX_FLOAT, std::complex<float>)
                        _KARABO_HELPER_MACRO(COMPLEX_DOUBLE, std::complex<double>)
                        _KARABO_HELPER_MACRO(STRING, std::string)
                        _KARABO_HELPER_MACRO(NONE, CppNone)
                    default:
                        throw KARABO_CAST_EXCEPTION("Casting of is not supported for target type");
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Problems with casting"));
            }
            #undef _KARABO_HELPER_MACRO
        }

        template<typename KeyType, typename AttributeType>
        inline std::string Element<KeyType, AttributeType>::getValueAsString() const {

            #define _KARABO_HELPER_MACRO(RefType, CppType)\
                    case Types::RefType: return karabo::util::toString(getValue<CppType>());\
                    case Types::VECTOR_##RefType: return karabo::util::toString(getValue<std::vector<CppType> >());

            Types::ReferenceType type = this->getType();
            switch (type) {
                    _KARABO_HELPER_MACRO(BOOL, bool)
                    _KARABO_HELPER_MACRO(CHAR, char)
                    _KARABO_HELPER_MACRO(INT8, signed char)
                    _KARABO_HELPER_MACRO(UINT8, unsigned char)
                    _KARABO_HELPER_MACRO(INT16, short)
                    _KARABO_HELPER_MACRO(UINT16, unsigned short)
                    _KARABO_HELPER_MACRO(INT32, int)
                    _KARABO_HELPER_MACRO(UINT32, unsigned int)
                    _KARABO_HELPER_MACRO(INT64, long long)
                    _KARABO_HELPER_MACRO(UINT64, unsigned long long)
                    _KARABO_HELPER_MACRO(FLOAT, float)
                    _KARABO_HELPER_MACRO(DOUBLE, double)
                    _KARABO_HELPER_MACRO(COMPLEX_FLOAT, std::complex<float>)
                    _KARABO_HELPER_MACRO(COMPLEX_DOUBLE, std::complex<double>)
                    _KARABO_HELPER_MACRO(STRING, std::string)
                    _KARABO_HELPER_MACRO(NONE, CppNone)        
                default:
                    throw KARABO_CAST_EXCEPTION("Could not convert value of key \"" + m_key + "\" to string");
            }
            #undef _KARABO_HELPER_MACRO
        }
    }
}
#endif

