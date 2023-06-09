/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 23, 2011, 11:12 AM
 *
 * Major re-design on February 1, 2013, 1:00 PM
 *
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

#ifndef KARABO_UTIL_OVERWRITEELEMENT_HH
#define KARABO_UTIL_OVERWRITEELEMENT_HH

#include "Schema.hh"


namespace karabo {
    namespace util {

        /**
         * @class OverwriteElement
         * @brief The OverwriteElement allows to overwrite/redefine Element properties
         * of an existing Element of a base class in a derived class.
         */
        class OverwriteElement {
           public:
            /** This class allows to define restrictions for the OVERWRITE_ELEMENT, i.e. which
             * attributes of a different element may not be altered through OVERWRITE. The
             * TABLE_ELEMENT e.g. has no notion of minInc or maxInc, and these should thus
             * not be settable to it via overwrites.
             *
             * The class "serializes" its options to a vector<bool> which can be passed as
             * an attribute to the node describing the element. It works by making use
             * of the ordered nature of the hash, allowing it to reconstruct itself from a
             * vector<bool>.
             */
            class Restrictions {
                friend class OverwriteElement;

#define KARABO_OVERWRITE_RESTRICTIONS "overwriteRestrictions"
               private:
                Hash::Pointer m_rest;

                /**
                 The Restriction struct resembles an individual restriction on the OVERWRITE_ELEMENT
                 * While restrictions are set through boolean assignment, its internal data structure
                 * is a Karabo Hash.
                 *
                 * This struct is fully private on purpose.
                 */
                struct Restriction {
                    std::string name;
                    Hash::Pointer m_restrictionsPtr;

                    Restriction(){};

                    Restriction(const std::string& name_, Hash::Pointer restrictionPtr, const bool def)
                        : name(name_), m_restrictionsPtr(restrictionPtr) {
                        m_restrictionsPtr->set(name, def);
                    };

                    Restriction& operator=(const bool rhs) {
                        m_restrictionsPtr->set(name, rhs);
                        return *this;
                    }

                    operator bool() const {
                        return m_restrictionsPtr->get<bool>(name);
                    }

                    bool operator!() const {
                        return !m_restrictionsPtr->get<bool>(name);
                    }
                };


               public:
                Restriction alias;
                Restriction displayedName;
                Restriction description;
                Restriction tag;
                Restriction assignmentMandatory;
                Restriction assignmentOptional;
                Restriction assignmentInternal;
                Restriction init;
                Restriction reconfigurable;
                Restriction readOnly;
                Restriction defaultValue;
                Restriction minInc;
                Restriction maxInc;
                Restriction minExc;
                Restriction maxExc;
                Restriction min;
                Restriction max;
                Restriction minSize;
                Restriction maxSize;
                Restriction options;
                Restriction stateOptions;
                Restriction allowedStates;
                Restriction observerAccess;
                Restriction userAccess;
                Restriction operatorAccess;
                Restriction expertAccess;
                Restriction adminAccess;
                Restriction skipValidation;
                Restriction unit;
                Restriction metricPrefix;
                Restriction overwriteRestrictions;

                /**
                 * Returns the set of a restrictions as a vector<bool> to be set to as an attribute.
                 * @return
                 */
                std::vector<bool> toVectorAttribute() const {
                    std::vector<bool> ret;
                    for (Hash::const_iterator it = m_rest->begin(); it != m_rest->end(); ++it) {
                        ret.push_back(it->getValue<bool>());
                    }
                    return ret;
                }

                /**
                 * Merges two sets of restrictions. Set restrictions from either element are preserved
                 * during the merge
                 * @param rhs: element to be merged
                 * @return: the merged element.
                 */
                Restrictions& merge(const Restrictions& rhs) {
                    for (Hash::const_iterator it = m_rest->begin(); it != m_rest->end(); ++it) {
                        it->setValue(it->getValue<bool>() || rhs.m_rest->get<bool>(it->getKey()));
                    }
                    return *this;
                }

                Restrictions()
                    : m_rest(new Hash),
                      alias(KARABO_SCHEMA_ALIAS, m_rest, false),
                      displayedName("displayedName", m_rest, false),
                      description("description", m_rest, false),
                      tag(KARABO_SCHEMA_TAGS, m_rest, false),
                      assignmentMandatory("assignmentMandatory", m_rest, false),
                      assignmentOptional("assignmentOptional", m_rest, false),
                      assignmentInternal("assignmentInternal", m_rest, false),
                      init("init", m_rest, false),
                      reconfigurable("reconfigurable", m_rest, false),
                      readOnly("readOnly", m_rest, false),
                      defaultValue("defaultValue", m_rest, false),
                      minInc("minInc", m_rest, false),
                      maxInc("maxInc", m_rest, false),
                      minExc("minExc", m_rest, false),
                      maxExc("maxExc", m_rest, false),
                      min("min", m_rest, false),
                      max("max", m_rest, false),
                      minSize("minSize", m_rest, false),
                      maxSize("maxSize", m_rest, false),
                      options("options", m_rest, false),
                      stateOptions("stateOptions", m_rest,
                                   true) // true by default as all elements but state are restricted
                      ,
                      allowedStates("allowedStates", m_rest, false),
                      observerAccess("observerAccess", m_rest, false),
                      userAccess("userAccess", m_rest, false),
                      operatorAccess("operatorAccess", m_rest, false),
                      expertAccess("expertAccess", m_rest, false),
                      adminAccess("adminAccess", m_rest, false),
                      skipValidation("skipValidation", m_rest, false),
                      unit("unit", m_rest, false),
                      metricPrefix("metricPrefix", m_rest, false),
                      overwriteRestrictions("overWriteRestrictions", m_rest, false) {}

                virtual ~Restrictions() {}

                /**
                 * Assigns from a vector<bool> indicating restrictions. Order of entries is in declaration order
                 * of restrictions.
                 * @param attrs
                 */
                void assignFromAttrVector(const std::vector<bool>& attrs) {
                    if (attrs.size() != m_rest->size()) {
                        throw KARABO_PARAMETER_EXCEPTION(
                              "Overwrite restrictions cannot be created from the passed attribute");
                    }
                    unsigned int i = 0;
                    for (Hash::iterator it = m_rest->begin(); it != m_rest->end(); ++it) {
                        it->setValue(attrs[i]);
                        i++;
                    }
                }
            };

            OverwriteElement(Schema& expected);
            /**
             * Specify the key to be overwritten
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name);

            /**
             * Set a new displayed name
             * @param name
             * @return
             */
            OverwriteElement& setNewDisplayedName(const std::string& name);

            /**
             * Set a new description
             * @param description
             * @return
             */
            OverwriteElement& setNewDescription(const std::string& description);

            /**
             * Set a new alias
             * @param alias
             * @return
             */
            template <class AliasType>
            OverwriteElement& setNewAlias(const AliasType& alias) {
                checkIfRestrictionApplies(m_restrictions.alias);
                if (m_node) m_node->setAttribute<AliasType>(KARABO_SCHEMA_ALIAS, alias);
                return *this;
            }

            /**
             * Set new tags
             * @param tags
             * @return
             */
            OverwriteElement& setNewTags(const std::vector<std::string>& tags) {
                checkIfRestrictionApplies(m_restrictions.tag);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_TAGS, tags);
                return *this;
            }

            /**
             * Set to now mandatory assignment
             * @return
             */
            OverwriteElement& setNewAssignmentMandatory();

            /**
             * Set to now optional assignment
             * @return
             */
            OverwriteElement& setNewAssignmentOptional();

            /**
             * Set to now internal assignment
             * @return
             */
            OverwriteElement& setNewAssignmentInternal();

            /**
             * Set to now being configurable only upon init
             * @return
             */
            OverwriteElement& setNowInit();

            /**
             * Set to now being reconfigurable
             * @return
             */
            OverwriteElement& setNowReconfigurable();

            /**
             * Set to now being read-only
             * @return
             */
            OverwriteElement& setNowReadOnly();

            /**
             * Set to now needing validation
             * @return
             */
            OverwriteElement& setNowValidate();

            /**
             * Set to now needing skipping validation for this element
             * @return
             */
            OverwriteElement& setNowSkipValidation();

            /**
             * Set a new default value for this element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.defaultValue);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, value);
                return *this;
            }

            /**
             * Set a new minimum inclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMinInc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minInc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_INC, value);
                return *this;
            }

            /**
             * Set a new maximum inclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMaxInc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxInc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_INC, value);
                return *this;
            }

            /**
             * Set a new minimum exclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMinExc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minExc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_EXC, value);
                return *this;
            }

            /**
             * Set a new maximum exclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMaxExc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxExc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_EXC, value);
                return *this;
            }

            /**
             * Set a new minimum restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMin(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.min);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN, value);
                return *this;
            }

            /**
             * Set a new maximum restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMax(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.max);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX, value);
                return *this;
            }

            /**
             * Set a new minimum size restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMinSize(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minSize);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            /**
             * Set a new maximum size restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMaxSize(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxSize);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            /**
             * Set new allowed options for this element
             * @param opts
             * @param sep
             * @return
             */
            OverwriteElement& setNewOptions(const std::string& opts, const std::string& sep = " ,;");
            OverwriteElement& setNewOptions(const std::vector<karabo::util::State>& opts);
            OverwriteElement& setNewOptions(const karabo::util::State& s1);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6,
                                            const karabo::util::State& s7);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6,
                                            const karabo::util::State& s7, const karabo::util::State& s8);
            OverwriteElement& setNewOptions(const std::vector<std::string>& opts);
            OverwriteElement& setNewAllowedStates(const std::vector<karabo::util::State>& states);
            // overloads for up to six states

            /**
             * Set new allowed States for this element
             * @param s1-6
             * @param sep
             * @return
             */
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                                  const karabo::util::State& s3);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                                  const karabo::util::State& s3, const karabo::util::State& s4);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                                  const karabo::util::State& s5);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                                  const karabo::util::State& s5, const karabo::util::State& s6);
            OverwriteElement& setNowObserverAccess();

            /**
             * Set the element to now have user access
             * @return
             */
            OverwriteElement& setNowUserAccess();

            /**
             * Set the element to now have operator access
             * @return
             */
            OverwriteElement& setNowOperatorAccess();

            /**
             * Set the element to now have expert access
             * @return
             */
            OverwriteElement& setNowExpertAccess();

            /**
             * Set the element to now have admin access
             * @return
             */
            OverwriteElement& setNowAdminAccess();

            /**
             * Set a new unit to use for values of this element
             * @param unit
             * @return
             */
            OverwriteElement& setNewUnit(const UnitType& unit);

            /**
             * Set a new metric prefix to use for values of this element
             * @param unit
             * @return
             */
            OverwriteElement& setNewMetricPrefix(const MetricPrefixType& metricPrefix);

            /**
             * Adds new restrictions to the element by merging with existing restrictions
             * @param restrictions, contains the new set of restrictions as determined after merging with existing ones
             * @return
             */
            OverwriteElement& setNewOverwriteRestrictions(OverwriteElement::Restrictions& restrictions);

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            void commit();

           private:
            /**
             * Throws an exemption if the restriction in question is set
             * @param restriction
             */
            void checkIfRestrictionApplies(const Restrictions::Restriction& restriction) const;

            OverwriteElement& setNewOptions(const std::string& opts, bool protect, const std::string& sep);

            Schema* m_schema;
            Hash::Node* m_node;
            std::string m_path;
            Restrictions m_restrictions;
        };

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::State>(
              const karabo::util::State& value) {
            return setNewDefaultValue(toString(value));
        }

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::AlarmCondition>(
              const karabo::util::AlarmCondition& value) {
            return setNewDefaultValue(value.asString());
        }


        typedef OverwriteElement OVERWRITE_ELEMENT;


    } // namespace util
} // namespace karabo
#endif
