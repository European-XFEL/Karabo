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
 * File:   Element.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on December 14, 2012, 11:07 AM
 */

#ifndef KARABO_UTIL_NODE_HH
#define KARABO_UTIL_NODE_HH

#include <any>
#include <boost/numeric/conversion/cast.hpp>
#include <complex>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "Exception.hh"
#include "FromTypeInfo.hh"
#include "MetaTools.hh"
#include "ToCppString.hh"
#include "Types.hh"


namespace karabo {
    namespace util {

        // Class forward (needed to prevent assignment of Hash to Attribute)
        class Hash;
        class Schema;

#define KARABO_HASH_CLASS_ID "__classId"

        template <class T>
        class GenericElement;

        /**
         * @class Element
         * @brief Class representing leaf elements in a Hash which may have
         *        attributes
         */
        template <typename KeyType, typename AttributesType = bool>
        class Element {
            // Grant friendship to container
            template <typename T, typename U>
            friend class OrderedMap;

            // Grant friendship to GenericElement (needs to setKey)
            template <class T>
            friend class GenericElement;

            // Members
            KeyType m_key;
            AttributesType m_attributes;
            std::any m_value;

            /**
             * Helper struct adding a classId attribute for classes inheriting (but not being) Hash.
             * See also its specialisation for isHashTheBase and ValueType.
             */
            template <typename ValueType, typename isHashTheBase>
            struct SetClassIdAttribute {
                // This generic implementation is for isHashTheBase = std::true_type and ValueType != Hash,
                // see specialisations for false_type and Hash below.
                /**
                 * Set the classId attribute as given by value.getClassInfo()
                 * @param value of classId to set
                 * @param e element to set classId to
                 */
                SetClassIdAttribute(const ValueType& value, Element& e) {
                    e.setAttribute(KARABO_HASH_CLASS_ID, value.getClassInfo().getClassId());
                }

                /**
                 * Helper to hack rvalue reference 'value' (which is Hash derived) back into a valid state
                 */
                void resetHashDerivedMoved(ValueType&& value) {
                    // Where SetClassIdAttribute was used, 'value' got moved from as a Hash. The Hash move semantics
                    // clears the Hash - which leaves the Hash-derived ValueType likely in an invalid state (missing
                    // keys). Cure that here by (move-)assigning a default contructed object. (Which means that
                    // Hash-derived objects have to have a default constructor...)
                    value = ValueType();
                }

                /**
                 * No-op for lvalue reference
                 */
                void resetHashDerivedMoved(ValueType& value) {}
            };

            /**
             * Types that are neither Hashes nor derived from Hashes are not touched.
             */
            template <typename ValueType>
            struct SetClassIdAttribute<ValueType, std::false_type> {
                /**
                 * For non Hash-derived types this is a no-op
                 * @param value
                 * @param e
                 */
                SetClassIdAttribute(const ValueType& value, Element& e) {
                    // Do nothing on purpose!
                }

                /**
                 * No-op
                 */
                void resetHashDerivedMoved(const ValueType& value) {}
            };

            /**
             * Hashes are not touched.
             */
            template <typename isHashTheBase>
            struct SetClassIdAttribute<Hash, isHashTheBase> {
                /**
                 * For the Hash itself (i.e. where std::is_base_of<Hash, Hash>::type is not std::false_type),
                 * this is a no-op as well.
                 * @param value
                 * @param e
                 */
                SetClassIdAttribute(const Hash& value, Element& e) {
                    // Do nothing on purpose!
                }

                void resetHashDerivedMoved(const Hash& value) {}
            };

           public:
            /**
             * Construct an empty Hash element
             */
            Element();

            /**
             * Construct a Hash element from a std::any value
             * @param key identifies the element
             * @param value of the element
             */
            Element(const KeyType& key, const std::any& value);

            /**
             * Construct a Hash element from a std::any value
             * @param key identifies the element
             * @param value of the element
             */
            Element(const KeyType& key, std::any&& value);

            /**
             * Construct a Hash element from an arbitrary value type
             * @param key identifies the element
             * @param value of the element
             */
            template <class ValueType>
            Element(const KeyType& key, const ValueType& value);

            /**
             * Construct a Hash element from an arbitrary value type
             * @param key identifies the element
             * @param value of the element
             */
            template <class ValueType>
            Element(const KeyType& key, ValueType&& value);

            // TODO Constructor with attributes container ??

            virtual ~Element();

            /**
             * Return the key identifying this Element
             * @return
             */
            const KeyType& getKey() const;

            /**
             * Set a value of arbitrary type to this Element
             * @param value
             */
            template <class ValueType>
            inline void setValue(const ValueType& value);

            /**
             * Set a value of arbitrary type to this Element
             * @param value
             */
            template <class ValueType>
            inline void setValue(ValueType&& value);

            /**
             * Set the value to a std::shared_ptr of ValueType
             * @param value
             */
            template <class ValueType>
            inline void setValue(const std::shared_ptr<ValueType>& value);

            /**
             *  For downward compatibility we allow insertion of
             * shared_ptr<Hash>. In general, we will create a compiler
             * error for all objects deriving from Hash and wrapped as shared pointer.
             */
            void setValue(const std::shared_ptr<Hash>& value);

            /**
             * Overload for setting char pointers (c-strings). Internally, the
             * element will hold a std::string.
             * @param value
             */
            void setValue(const char* value);

            /**
             * Overload for setting char pointers (c-strings). Internally, the
             * element will hold a std::string.
             * @param value
             */
            void setValue(char* value);

            /**
             * Overload for setting wide char pointers (c-strings). Internally, the
             * element will hold a std::wstring.
             * @param value
             */
            void setValue(const wchar_t* value);

            /**
             * Overload for setting wide char pointers (c-strings). Internally, the
             * element will hold a std::wstring.
             * @param value
             */
            void setValue(wchar_t* value);

            /**
             * Set a std::any value to the Element
             * @param value
             */
            void setValue(const std::any& value);

            /**
             * Set a std::any value to the Element
             * @param value
             */
            void setValue(std::any&& value);

            /**
             * Set the value of another element to this Element, key and attributes are unchanged.
             *
             * Kept for backward compatibility, better use setValue(other.getValueAsAny()) instead.
             *
             * @param other
             */
            void setValue(const Element<KeyType, AttributesType>& other);

            /**
             * Set the value of another element to this Element, key and attributes are unchanged.
             *
             * Kept for backward compatibility, better use setValue(std::move(other.getValueAsAny())) instead.
             *
             * @param other
             */
            void setValue(Element<KeyType, AttributesType>&& other);

            /**
             * Return the first successful cast to one of the ValueTypes
             * (DestValueType, SourceValueType or one of the SourceValueTypes).
             * Strict casting is applied,
             * i.e. at least one of the ValueTypes needs to be of the exact type
             * of inserted value (or implicitly castable)
             * @return DestValueType by "copy". Candidate for Return Value Optimization through copy elision.
             */
            template <class DestValueType, class SourceValueType, class... SourceValueTypes>
            inline DestValueType getValue() const;

            /**
             * Return the value cast to ValueType. Strict casting is applied,
             * i.e. the ValueType needs to be of the exact type of inserted
             * value (or implicitly castable)
             * @return
             */
            template <class ValueType>
            inline const ValueType& getValue() const;

            /**
             * Return the value cast to ValueType. Strict casting is applied,
             * i.e. the ValueType needs to be of the exact type of inserted
             * value (or implicitly castable)
             * @return
             */
            template <class ValueType>
            inline ValueType& getValue();

            /**
             * Return the value as std::any. Does not throw
             * @return
             */
            std::any& getValueAsAny();

            /**
             * Return the value as std::any. Does not throw
             * @return
             */
            const std::any& getValueAsAny() const;

            /**
             * Return the value cast to ValueType. Casting is performed via
             * string literal casts, i.e. less strict.
             * @return
             */
            template <typename ValueType>
            ValueType getValueAs() const;

            /**
             * Return the value cast to ValueType. Casting is performed via
             * string literal casts, i.e. less strict. Overload for vector-type
             * values
             * @return
             */
            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            Cont<T> getValueAs() const;

            /**
             * Return the value cast to string.
             * The only difference to getValueAs<string>() concerns elements of type Types::ReferenceType::VECTOR_*:
             * Whereas getValueAs<string>() returns all vector elements, getValueAsShortString() shortens the string by
             * leaving out vector elements in the middle, if the vector size
             * exceeds the argument.
             * @param maxNumVectorElements maximum number of vector elements taken into account
             * @return
             */
            std::string getValueAsShortString(size_t maxNumVectorElements) const;

            /**
             * Set an attribute to this Element, identified by key
             * @param key
             * @param value
             */
            template <class T>
            inline void setAttribute(const std::string& key, const T& value);

            /**
             * Set an attribute to this Element, identified by key
             * @param key
             * @param value
             */
            template <class T>
            inline void setAttribute(const std::string& key, T&& value);

            /**
             * Return the attribute cast to ValueType. Strict casting is applied,
             * i.e. the T needs to be of the exact type of inserted
             * vale (or implicitly castable)
             * @param key identifying the attribute
             * @return
             */
            template <class T>
            inline T& getAttribute(const std::string& key);

            /**
             * Return the attribute cast to ValueType. Strict casting is applied,
             * i.e. the T needs to be of the exact type of inserted
             * vale (or implicitly castable)
             * @param key identifying the attribute
             * @param value reference to insert value in
             * @return
             */
            template <class T>
            inline void getAttribute(const std::string& key, T& value) const;

            /**
             * Return the attribute cast to ValueType. Strict casting is applied,
             * i.e. the T needs to be of the exact type of inserted
             * vale (or implicitly castable)
             * @param key identifying the attribute
             * @return
             */
            template <class T>
            inline const T& getAttribute(const std::string& key) const;

            /**
             * Return the attribute cast to ValueType. Strict casting is applied,
             * i.e. the T needs to be of the exact type of inserted
             * vale (or implicitly castable)
             * @param key identifying the attribute
             * @param value reference to insert value in
             * @return
             */
            template <class T>
            inline void getAttribute(const std::string& key, const T& value) const;

            /**
             * Return the value as std::any. Does not throw
             * @param key identifying the attribute
             * @return
             */
            inline const std::any& getAttributeAsAny(const std::string& key) const;

            /**
             * Return the value as std::any. Does not throw
             * @param key identifying the attribute
             * @return
             */
            inline std::any& getAttributeAsAny(const std::string& key);

            /**
             * Return the attribute cast to ValueType. Casting is performed via
             * string literal casts, i.e. less strict.
             * param key identifying the attribute
             * @return
             */
            template <class T>
            inline T getAttributeAs(const std::string& key) const;

            /**
             * Return the attribute cast to ValueType. Casting is performed via
             * string literal casts, i.e. less strict. Overload for vector-type
             * values
             * param key identifying the attribute
             * @return
             */
            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            inline Cont<T> getAttributeAs(const std::string& key) const;

            /**
             * Return an attribute as a Node, e.g. an Element<T>
             * @param key
             * @return
             */
            Element<KeyType>& getAttributeNode(const std::string& key);

            /**
             * Return an attribute as a Node, e.g. an Element<T>
             * @param key
             * @return
             */
            const Element<KeyType>& getAttributeNode(const std::string& key) const;

            /**
             * Check if Element has an attribute identified by key
             * @param key
             * @return true if the attribute exists, false if not
             */
            inline bool hasAttribute(const std::string& key) const;

            /**
             * Batch set attributes to this element
             * @param attributes
             */
            inline void setAttributes(const AttributesType& attributes);

            /**
             * Batch set attributes to this element
             * @param attributes to move from
             */
            inline void setAttributes(AttributesType&& attributes);

            /**
             * Batch get attributes of this element
             * @return
             */
            inline const AttributesType& getAttributes() const;

            /**
             * Batch get attributes of this element
             * @return
             */
            inline AttributesType& getAttributes();

            /**
             * Check if element is of type T
             * @return true if element is type T
             */
            template <typename T>
            inline bool is() const;

            /**
             * Return the type of this element as a Karabo reference type
             * @return
             */
            Types::ReferenceType getType() const;

            /**
             * Return the std::type_info struct for this element' type
             * @return
             */
            const std::type_info& type() const;

            /**
             * Set the type of this Element to a different type. Requires that
             * non-strict casting, as for getValueAs is possible. Otherwise
             * throws and exception
             * @param tgtType type to set the element to
             */
            void setType(const Types::ReferenceType& tgtType);

            /**
             * Compare two elements for equality: Checks if the elements have
             * the same key
             * @param other
             * @return
             */
            bool operator==(const Element<KeyType, AttributesType>& other) const;

            /**
             * Compare two elements for inequality: Checks if the elements have
             * the same key
             * @param other
             * @return
             */
            bool operator!=(const Element<KeyType, AttributesType>& other) const;

           private:
            template <class ValueType, typename is_hash_the_base>
            void setValue(const ValueType& value);


            template <class ValueType, typename is_hash_the_base>
            void setValue(ValueType&& value);


            template <class ValueType>
            inline const ValueType& getValue(std::true_type /*is_hash_the_base*/) const;

            template <class ValueType>
            inline const ValueType& getValue(std::false_type /*is_hash_the_base*/) const;

            inline void setKey(const KeyType& key);

            inline std::string getValueAsString() const;
        };

        /**********************************************************************
         *
         * Implementation Node
         *
         **********************************************************************/

        // TODO This should be implemented in a cleaner way

        template <class KeyType, typename AttributeType>
        Element<KeyType, AttributeType>::Element() {}

        template <class KeyType, typename AttributeType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, const std::any& value)
            : m_key(key), m_value(value) {}

        template <class KeyType, typename AttributeType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, std::any&& value)
            : m_key(key), m_value(std::move(value)) {}

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, const ValueType& value) : m_key(key) {
            this->setValue(value);
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        Element<KeyType, AttributeType>::Element(const KeyType& key, ValueType&& value) : m_key(key) {
            this->setValue(std::forward<ValueType>(value));
        }

        template <class KeyType, typename AttributesType>
        Element<KeyType, AttributesType>::~Element() {}

        template <class KeyType, typename AttributeType>
        inline const KeyType& Element<KeyType, AttributeType>::getKey() const {
            return m_key;
        }

        template <class KeyType, typename AttributeType>
        void Element<KeyType, AttributeType>::setKey(const KeyType& key) {
            // could overload for 'KeyType&& key'
            m_key = key;
        }

        template <class KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::operator==(const Element<KeyType, AttributeType>& other) const {
            return this->m_key == other.m_key;
        }

        template <class KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::operator!=(const Element<KeyType, AttributeType>& other) const {
            return this->m_key != other.m_key;
        }

        template <class KeyType, typename AttributeType>
        template <typename T>
        inline bool Element<KeyType, AttributeType>::is() const {
            return m_value.type() == typeid(T);
        }

        template <class KeyType, typename AttributeType>
        Types::ReferenceType Element<KeyType, AttributeType>::getType() const {
            return FromType<FromTypeInfo>::from(m_value.type());
        }

        template <class KeyType, typename AttributeType>
        const std::type_info& Element<KeyType, AttributeType>::type() const {
            return m_value.type();
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline void Element<KeyType, AttributeType>::setValue(const ValueType& value) {
            this->setValue<ValueType, typename std::is_base_of<Hash, ValueType>::type>(value);
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline void Element<KeyType, AttributeType>::setValue(
              ValueType&& value) { // 'ValueType&&' is a universal reference!
            // forward returns either 'ValueType&&', 'ValueType&', or 'const ValueType&'
            using removed_reference = typename std::remove_reference<ValueType>::type;
            using removed_const_and_ref = typename std::remove_const<removed_reference>::type;
            this->setValue<decltype(std::forward<ValueType>(value)),
                           typename std::is_base_of<Hash, removed_const_and_ref>::type>(std::forward<ValueType>(value));
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline void Element<KeyType, AttributeType>::setValue(const std::shared_ptr<ValueType>& value) {
            this->setValue<std::shared_ptr<ValueType>, typename std::is_base_of<Hash, ValueType>::type>(value);
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType, typename is_hash_the_base>
        void Element<KeyType, AttributeType>::setValue(const ValueType& value) {
            m_value = conditional_hash_cast<is_hash_the_base>::cast(value);
            // When ValueType is derived from Hash, this sets the attribute __classId to the class id name.
            // If ValueType _is_ Hash or any class not inheriting from Hash, nothing is done.
            SetClassIdAttribute<typename std::remove_reference<ValueType>::type, is_hash_the_base>(value, *this);
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType, typename is_hash_the_base>
        void Element<KeyType, AttributeType>::setValue(ValueType&& value) {
            m_value = conditional_hash_cast<is_hash_the_base>::cast(std::forward<ValueType>(value));
            // When ValueType is derived from Hash, this sets the attribute __classId to the class id name.
            // If ValueType _is_ Hash or any class not inheriting from Hash, nothing is done.
            SetClassIdAttribute<typename std::remove_reference<ValueType>::type, is_hash_the_base> helper(value, *this);
            // If value is a Hash derived non-Hash class and we forward it as a real lvalue reference,
            // the move assignment to Hash clears the Hash container of 'value'. Very likely 'value' is NOT anymore
            // in a valid state: if 'value' is an NDArray, value.getShape() will throw since key "shape" is missing!
            // This is fixed here:
            helper.resetHashDerivedMoved(std::forward<ValueType>(value));
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const std::shared_ptr<Hash>& value) {
            m_value = value;
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const char* value) {
            m_value = std::string(value ? value : "");
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(char* value) {
            m_value = std::string(value ? value : "");
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const wchar_t* value) {
            m_value = std::wstring(value);
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(wchar_t* value) {
            m_value = std::wstring(value);
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const std::any& value) {
            m_value = value;
        }

        template <class KeyType, class AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(std::any&& value) {
            m_value = std::move(value);
        }

        template <class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(const Element<KeyType, AttributeType>& other) {
            if (this != &other) {
                this->m_value = other.m_value;
            }
        }

        template <class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setValue(Element<KeyType, AttributeType>&& other) {
            if (this != &other) {
                this->m_value = std::move(other.m_value);
            }
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline const ValueType& Element<KeyType, AttributeType>::getValue() const {
            return getValue<ValueType>(typename std::is_base_of<Hash, ValueType>::type());
        }

        template <class KeyType, typename AttributeType>
        template <class DestValueType, class SourceValueType, class... SourceValueTypes>
        inline DestValueType Element<KeyType, AttributeType>::getValue() const {
            // First try to cast from std::any to the destination type
            const SourceValueType* ptr = std::any_cast<SourceValueType>(&m_value);

            if (ptr) {
                if (std::is_arithmetic<DestValueType>::value && std::is_arithmetic<SourceValueType>::value) {
                    // Use boost::numeric_cast to filter out losses of range
                    // and reject conversions from negative value to unsigned types
                    try {
                        return boost::numeric_cast<DestValueType>(*ptr);
                    } catch (boost::numeric::bad_numeric_cast& e) {
                        KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(
                              karabo::util::createCastFailureMessage(m_key, m_value.type(), typeid(DestValueType))));
                    }
                }
                // Not arithmetic types, try a implicit static cast.
                return *ptr;
            }

            // The current source type is not the right one, proceed with the rest of the list
            return getValue<DestValueType, SourceValueTypes...>();
        }


        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline ValueType& Element<KeyType, AttributeType>::getValue() {
            return const_cast<ValueType&>(static_cast<const Element*>(this)->getValue<ValueType>(
                  typename std::is_base_of<Hash, ValueType>::type()));
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline const ValueType& Element<KeyType, AttributeType>::getValue(std::true_type /*is_hash_the_base*/) const {
            const Hash* ptr = std::any_cast<Hash>(&m_value);
            if (ptr) return reinterpret_cast<const ValueType&>(*ptr);
            throw KARABO_CAST_EXCEPTION(
                  karabo::util::createTypeMismatchMessage(m_key, m_value.type(), typeid(ValueType)));
        }

        template <class KeyType, typename AttributeType>
        template <class ValueType>
        inline const ValueType& Element<KeyType, AttributeType>::getValue(std::false_type /*is_hash_the_base*/) const {
            const ValueType* ptr = std::any_cast<ValueType>(&m_value);
            if (ptr) return *ptr;
            throw KARABO_CAST_EXCEPTION(
                  karabo::util::createTypeMismatchMessage(m_key, m_value.type(), typeid(ValueType)));
        }

        template <class KeyType, typename AttributeType>
        template <typename ValueType>
        inline ValueType Element<KeyType, AttributeType>::getValueAs() const {
            if (m_value.type() == typeid(ValueType)) return this->getValue<ValueType>();

            Types::ReferenceType srcType = this->getType();
            Types::ReferenceType tgtType = Types::from<ValueType>();

            if (srcType == Types::UNKNOWN)
                throw KARABO_CAST_EXCEPTION("Unknown source type for key: \"" + m_key +
                                            "\". Cowardly refusing to cast.");

            try {
                // Avoid extra copy if source is already string:
                const std::string& value =
                      (srcType == Types::STRING ? this->getValue<std::string>() : this->getValueAsString());
                return ValueType(karabo::util::fromString<ValueType>(value));
            } catch (...) {
                KARABO_RETHROW_AS(
                      KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, srcType, tgtType) +=
                                            " (string '" + this->getValueAsString() += "')"));
            }
            return ValueType();
        }

        template <class KeyType, typename AttributeType>
        template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
        inline Cont<T> Element<KeyType, AttributeType>::getValueAs() const {
            Types::ReferenceType srcType = this->getType();
            Types::ReferenceType tgtType = Types::from<Cont<T> >();

            if (tgtType == srcType) return this->getValue<Cont<T> >();
            if (srcType == Types::UNKNOWN)
                throw KARABO_CAST_EXCEPTION("Unknown source type for key: \"" + m_key +
                                            "\". Cowardly refusing to cast.");

            try {
                // Avoid extra copy if source is already string:
                const std::string& value =
                      (srcType == Types::STRING ? this->getValue<std::string>() : this->getValueAsString());

                if (value.empty()) return Cont<T>();

                return karabo::util::fromString<T, Cont>(value);
            } catch (...) {
                KARABO_RETHROW_AS(
                      KARABO_CAST_EXCEPTION(karabo::util::createCastFailureMessage(m_key, srcType, tgtType) +=
                                            " (string '" + this->getValueAsString() += "')"));
                return Cont<T>(); // Make the compiler happy
            }
        }

        template <class KeyType, typename AttributeType>
        std::string Element<KeyType, AttributeType>::getValueAsShortString(size_t maxNumVectorElements) const {
#define CASE_RETURN_VECTOR(VectorRefType, ElementCppType, maxSize) \
    case Types::ReferenceType::VectorRefType:                      \
        return karabo::util::toString(this->getValueAs<ElementCppType, std::vector>(), maxSize);

            switch (this->getType()) {
                // Not treating (VECTOR_CHAR, char) here: That is our raw data container treated elsewhere.
                CASE_RETURN_VECTOR(VECTOR_INT8, signed char, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_INT16, short, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_INT32, int, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_INT64, long long, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_UINT8, unsigned char, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_UINT16, unsigned short, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_UINT32, unsigned int, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_UINT64, unsigned long long, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_FLOAT, float, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_DOUBLE, double, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_BOOL, bool, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_STRING, std::string, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_COMPLEX_FLOAT, std::complex<float>, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_COMPLEX_DOUBLE, std::complex<double>, maxNumVectorElements)
                CASE_RETURN_VECTOR(VECTOR_NONE, CppNone, maxNumVectorElements) // for completeness
                case Types::ReferenceType::BYTE_ARRAY:
                    return karabo::util::toString(this->getValue<ByteArray>(), maxNumVectorElements);
                default:
                    return this->getValueAs<std::string>();
            }
#undef CASE_RETURN_VECTOR
        }


        template <class KeyType, typename AttributeType>
        std::any& Element<KeyType, AttributeType>::getValueAsAny() {
            return m_value;
        }

        template <class KeyType, typename AttributeType>
        const std::any& Element<KeyType, AttributeType>::getValueAsAny() const {
            return m_value;
        }

        template <class KeyType, typename AttributeType>
        inline const AttributeType& Element<KeyType, AttributeType>::getAttributes() const {
            return m_attributes;
        }

        template <class KeyType, typename AttributeType>
        inline AttributeType& Element<KeyType, AttributeType>::getAttributes() {
            return m_attributes;
        }

        template <class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttributes(const AttributeType& attributes) {
            m_attributes = attributes;
        }

        template <class KeyType, typename AttributeType>
        inline void Element<KeyType, AttributeType>::setAttributes(AttributeType&& attributes) {
            m_attributes = std::move(attributes);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, const T& value) {
            m_attributes.template set<T>(key, value);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline void Element<KeyType, AttributeType>::setAttribute(const std::string& key, T&& value) {
            m_attributes.set(key, std::forward<T>(value));
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline T& Element<KeyType, AttributeType>::getAttribute(const std::string& key) {
            return m_attributes.template get<T>(key);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline void Element<KeyType, AttributeType>::getAttribute(const std::string& key, T& value) const {
            m_attributes.template get(key, value);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline const T& Element<KeyType, AttributeType>::getAttribute(const std::string& key) const {
            return m_attributes.template get<T>(key);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline void Element<KeyType, AttributeType>::getAttribute(const std::string& key, const T& value) const {
            m_attributes.template get(key, value);
        }

        template <typename KeyType, typename AttributeType>
        inline const std::any& Element<KeyType, AttributeType>::getAttributeAsAny(const std::string& key) const {
            return m_attributes.getAny(key);
        }

        template <typename KeyType, typename AttributeType>
        inline std::any& Element<KeyType, AttributeType>::getAttributeAsAny(const std::string& key) {
            return m_attributes.getAny(key);
        }

        template <typename KeyType, typename AttributeType>
        template <class T>
        inline T Element<KeyType, AttributeType>::getAttributeAs(const std::string& key) const {
            return m_attributes.template getAs<T>(key);
        }

        template <typename KeyType, typename AttributeType>
        template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
        inline Cont<T> Element<KeyType, AttributeType>::getAttributeAs(const std::string& key) const {
            return m_attributes.template getAs<T, Cont>(key);
        }

        template <typename KeyType, typename AttributeType>
        inline Element<KeyType>& Element<KeyType, AttributeType>::getAttributeNode(const std::string& key) {
            return m_attributes.getNode(key);
        }

        template <typename KeyType, typename AttributeType>
        inline const Element<KeyType>& Element<KeyType, AttributeType>::getAttributeNode(const std::string& key) const {
            return m_attributes.getNode(key);
        }

        template <typename KeyType, typename AttributeType>
        bool Element<KeyType, AttributeType>::hasAttribute(const std::string& key) const {
            return m_attributes.has(key);
        }

        template <typename KeyType, typename AttributeType>
        struct SetType {
            inline SetType(Element<KeyType, AttributeType>& element, std::any& value)
                : m_element(element), m_value(value) {}

            template <class T>
            inline void operator()(T*) {
                m_value = m_element.template getValueAs<T>();
            }

            template <class T>
            inline void operator()(std::vector<T>*) {
                m_value = m_element.template getValueAs<T, std::vector>();
            }

            Element<KeyType, AttributeType>& m_element;
            std::any& m_value;
        };

        template <typename KeyType, typename AttributeType>
        void Element<KeyType, AttributeType>::setType(const Types::ReferenceType& tgtType) {
            Types::ReferenceType srcType = this->getType();
            if (tgtType == srcType) return;
            SetType<KeyType, AttributeType> processor(*this, m_value);
            if (templatize(tgtType, processor)) return;
            try {
                switch (tgtType) {
                    case Types::NONE:
                        m_value = getValueAs<CppNone>();
                        break;
                    case Types::VECTOR_NONE:
                        m_value = getValueAs<CppNone, std::vector>();
                        break;
                    case Types::BYTE_ARRAY:
                        m_value = this->getValueAs<karabo::util::ByteArray>();
                        break;
                    default:
                        throw KARABO_CAST_EXCEPTION("Casting of '" + Types::to<ToCppString>(srcType) +=
                                                    "' to '" + Types::to<ToCppString>(tgtType) += "' is not supported");
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Problems with casting"));
            }
        }

        template <typename KeyType, typename AttributeType>
        struct GetValueAsString {
            inline GetValueAsString(const Element<KeyType, AttributeType>& element, std::string& result)
                : m_element(element), m_result(result) {}

            template <class T>
            inline void operator()(T*) {
                m_result = karabo::util::toString(m_element.template getValue<T>());
            }

            const Element<KeyType, AttributeType>& m_element;
            std::string& m_result;
        };

        template <typename KeyType, typename AttributeType>
        inline std::string Element<KeyType, AttributeType>::getValueAsString() const {
            Types::ReferenceType type = this->getType();
            std::string result;
            GetValueAsString<KeyType, AttributeType> processor(*this, result);
            if (templatize(type, processor)) {
                return result;
            }
            switch (type) {
                case Types::HASH:
                    return karabo::util::toString(getValue<Hash>());
                case Types::VECTOR_HASH:
                    return karabo::util::toString(getValue<std::vector<Hash> >());
                case Types::NONE:
                    return karabo::util::toString(getValue<CppNone>());
                case Types::VECTOR_NONE:
                    return karabo::util::toString(getValue<std::vector<CppNone> >());
                case Types::SCHEMA:
                    return karabo::util::toString(getValue<Schema>());
                case Types::BYTE_ARRAY: {
                    const karabo::util::ByteArray& array = getValue<karabo::util::ByteArray>();
                    const unsigned char* data = reinterpret_cast<unsigned char*>(array.first.get());
                    return karabo::util::base64Encode(data, array.second);
                }
                default:
                    throw KARABO_CAST_EXCEPTION("Could not convert value of key \"" + m_key + "\" to string");
            }
        }
    } // namespace util
} // namespace karabo
#endif
