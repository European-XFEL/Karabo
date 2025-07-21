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

#ifndef KARABO_DATA_SCHEMA_OVERWRITEELEMENT_HH
#define KARABO_DATA_SCHEMA_OVERWRITEELEMENT_HH

#include "karabo/data/types/Schema.hh"


namespace karabo {
    namespace data {

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
                Restriction operatorAccess;
                Restriction expertAccess;
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
                      operatorAccess("operatorAccess", m_rest, false),
                      expertAccess("expertAccess", m_rest, false),
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
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.alias);
                m_node->setAttribute<AliasType>(KARABO_SCHEMA_ALIAS, alias);
                return *this;
            }

            /**
             * Set new tags
             * @param tags
             * @return
             */
            OverwriteElement& setNewTags(const std::vector<std::string>& tags) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.tag);
                m_node->setAttribute(KARABO_SCHEMA_TAGS, tags);
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
             * Set a new default value for this element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.defaultValue);
                m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, value);
                return *this;
            }

            /**
             * Set a new minimum inclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMinInc(const ValueType& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.minInc);
                m_node->setAttribute(KARABO_SCHEMA_MIN_INC, value);
                return *this;
            }

            /**
             * Set a new maximum inclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMaxInc(const ValueType& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.maxInc);
                m_node->setAttribute(KARABO_SCHEMA_MAX_INC, value);
                return *this;
            }

            /**
             * Set a new minimum exclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMinExc(const ValueType& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.minExc);
                m_node->setAttribute(KARABO_SCHEMA_MIN_EXC, value);
                return *this;
            }

            /**
             * Set a new maximum exclusive restriction for values set to the element
             * @param value
             * @return
             */
            template <class ValueType>
            OverwriteElement& setNewMaxExc(const ValueType& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.maxExc);
                m_node->setAttribute(KARABO_SCHEMA_MAX_EXC, value);
                return *this;
            }

            /**
             * Set a new minimum size restriction for values set to the element
             * @param value
             * @return
             */
            OverwriteElement& setNewMinSize(const unsigned int& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.minSize);
                m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            /**
             * Set a new maximum size restriction for values set to the element
             * @param value
             * @return
             */
            OverwriteElement& setNewMaxSize(const unsigned int& value) {
                if (!m_node) {
                    throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
                }
                checkIfRestrictionApplies(m_restrictions.maxSize);
                m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            /**
             * Set new allowed options for this element
             * @param opts
             * @param sep
             * @return
             */
            OverwriteElement& setNewOptions(const std::string& opts, const std::string& sep = " ,;");
            OverwriteElement& setNewOptions(const std::vector<karabo::data::State>& opts);
            OverwriteElement& setNewOptions(const karabo::data::State& s1);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3, const karabo::data::State& s4);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3, const karabo::data::State& s4,
                                            const karabo::data::State& s5);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3, const karabo::data::State& s4,
                                            const karabo::data::State& s5, const karabo::data::State& s6);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3, const karabo::data::State& s4,
                                            const karabo::data::State& s5, const karabo::data::State& s6,
                                            const karabo::data::State& s7);
            OverwriteElement& setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                            const karabo::data::State& s3, const karabo::data::State& s4,
                                            const karabo::data::State& s5, const karabo::data::State& s6,
                                            const karabo::data::State& s7, const karabo::data::State& s8);
            OverwriteElement& setNewOptions(const std::vector<std::string>& opts);
            OverwriteElement& setNewAllowedStates(const std::vector<karabo::data::State>& states);
            // overloads for up to six states

            /**
             * Set new allowed States for this element
             * @param s1-6
             * @param sep
             * @return
             */
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1);
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1, const karabo::data::State& s2);
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                                  const karabo::data::State& s3);
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                                  const karabo::data::State& s3, const karabo::data::State& s4);
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                                  const karabo::data::State& s3, const karabo::data::State& s4,
                                                  const karabo::data::State& s5);
            OverwriteElement& setNewAllowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                                  const karabo::data::State& s3, const karabo::data::State& s4,
                                                  const karabo::data::State& s5, const karabo::data::State& s6);
            OverwriteElement& setNowObserverAccess();

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
             * Set a new unit to use for values of this element
             * @param unit
             * @return
             */
            OverwriteElement& setNewUnit(const Unit& unit);

            /**
             * Set a new metric prefix to use for values of this element
             * @param unit
             * @return
             */
            OverwriteElement& setNewMetricPrefix(const MetricPrefix& metricPrefix);

            /**
             * Adds new restrictions to the element by merging with existing restrictions
             * @param restrictions, contains the new set of restrictions as determined after merging with existing ones
             * @return
             */
            OverwriteElement& setNewOverwriteRestrictions(OverwriteElement::Restrictions& restrictions);

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the BaseElement
             */
            void commit();

           private:
            /**
             * Throws an exemption if the restriction in question is set
             * @param restriction
             */
            void checkIfRestrictionApplies(const Restrictions::Restriction& restriction) const;

            OverwriteElement& setNewOptions(const std::string& opts, bool protect, const std::string& sep);

            // Called by commit, to check value/options consistency
            void checkOptions();

            // Called by commit, to check min/default/max consistency
            void checkBoundaries();

            template <typename T>
            void checkMinMax() {
                if (m_schema->hasMinInc(m_path)) {
                    if (m_schema->hasMaxInc(m_path) &&
                        m_schema->getMinIncAs<T>(m_path) > m_schema->getMaxIncAs<T>(m_path)) {
                        throw KARABO_PARAMETER_EXCEPTION(
                              "Inclusive minimum (" + m_schema->getMinIncAs<std::string>(m_path) + ") for " + m_path +
                              " greater than inclusive maximum (" + m_schema->getMaxIncAs<std::string>(m_path) + ")");
                    }

                    if (m_schema->hasMaxExc(m_path) &&
                        m_schema->getMinIncAs<T>(m_path) >= m_schema->getMaxExcAs<T>(m_path)) {
                        throw KARABO_PARAMETER_EXCEPTION("Inclusive minimum (" +
                                                         m_schema->getMinIncAs<std::string>(m_path) + ") for " +
                                                         m_path + " greater than or equal to exclusive maximum (" +
                                                         m_schema->getMaxExcAs<std::string>(m_path) + ")");
                    }
                }


                if (m_schema->hasMinExc(m_path)) {
                    if (m_schema->hasMaxExc(m_path) &&
                        m_schema->getMinExcAs<T>(m_path) >= m_schema->getMaxExcAs<T>(m_path)) {
                        throw KARABO_PARAMETER_EXCEPTION("Exclusive minimum (" +
                                                         m_schema->getMinExcAs<std::string>(m_path) + ") for " +
                                                         m_path + " greater than or equal to exclusive maximum (" +
                                                         m_schema->getMaxExcAs<std::string>(m_path) + ")");
                    }

                    if (m_schema->hasMaxInc(m_path) &&
                        m_schema->getMinExcAs<T>(m_path) >= m_schema->getMaxIncAs<T>(m_path)) {
                        throw KARABO_PARAMETER_EXCEPTION(
                              "Exclusive minimum (" + m_schema->getMinExcAs<std::string>(m_path) + ") for " + m_path +
                              " greater than inclusive maximum (" + m_schema->getMaxIncAs<std::string>(m_path) + ")");
                    }
                }
            }

            /** Check default/max/min value consistency, after the value type is known
             * Requires that m_node is not NULL
             */

            template <typename T>
            void checkTypedBoundaries() {
                if (!m_schema->hasDefaultValue(m_path)) {
                    checkMinMax<T>();
                    return;
                }

                const T default_value = m_schema->getDefaultValueAs<T>(m_path);

                if (m_schema->hasMinInc(m_path) && default_value < m_schema->getMinIncAs<T>(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "Default value (" + m_schema->getDefaultValueAs<std::string>(m_path) + ") for " + m_path +
                          " smaller than inclusive minimum (" + m_schema->getMinIncAs<std::string>(m_path) + ")");
                }

                if (m_schema->hasMaxInc(m_path) && default_value > m_schema->getMaxIncAs<T>(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "Default value (" + m_schema->getDefaultValueAs<std::string>(m_path) + ") for " + m_path +
                          " greater than inclusive maximum (" + m_schema->getMaxIncAs<std::string>(m_path) + ")");
                }

                if (m_schema->hasMinExc(m_path) && default_value <= m_schema->getMinExcAs<T>(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION("Default value (" +
                                                     m_schema->getDefaultValueAs<std::string>(m_path) + ") for " +
                                                     m_path + " smaller than or equal to exclusive minimum (" +
                                                     m_schema->getMinExcAs<std::string>(m_path) + ")");
                }

                if (m_schema->hasMaxExc(m_path) && default_value >= m_schema->getMaxExcAs<T>(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION("Default value (" +
                                                     m_schema->getDefaultValueAs<std::string>(m_path) + ") for " +
                                                     m_path + " greater than or equal to exclusive maximum (" +
                                                     m_schema->getMaxExcAs<std::string>(m_path) + ")");
                }
            }

            template <typename T>
            void checkMinMaxSize() {
                if (m_schema->hasMinSize(m_path) && m_schema->hasMaxSize(m_path) &&
                    m_schema->getMinSize(m_path) > m_schema->getMaxSize(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION("Minimum size (" + std::to_string(m_schema->getMinSize(m_path)) +
                                                     ") for " + m_path + " greater than maximum size (" +
                                                     std::to_string(m_schema->getMaxSize(m_path)) + ")");
                }
            }

            template <typename T>
            void checkVectorBoundaries() {
                if (!m_schema->hasDefaultValue(m_path)) {
                    checkMinMaxSize<T>();
                    return;
                }

                const std::vector<T>& default_vector = m_schema->getDefaultValue<std::vector<T>>(m_path);
                const size_t default_size = default_vector.size();

                if (m_schema->hasMinSize(m_path) && default_size < m_schema->getMinSize(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION("Default size (" + std::to_string(default_size) + ") for " +
                                                     m_path + " less than minimum size (" +
                                                     std::to_string(m_schema->getMinSize(m_path)) + ")");
                }

                if (m_schema->hasMaxSize(m_path) && default_size > m_schema->getMaxSize(m_path)) {
                    throw KARABO_PARAMETER_EXCEPTION("Default size (" + std::to_string(default_size) + ") for " +
                                                     m_path + " greater than maximum size (" +
                                                     std::to_string(m_schema->getMaxSize(m_path)) + ")");
                }
            }

            Schema* m_schema;
            Hash::Node* m_node;
            std::string m_path;
            Restrictions m_restrictions;
        };

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::data::State>(
              const karabo::data::State& value) {
            return setNewDefaultValue(toString(value));
        }

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::data::AlarmCondition>(
              const karabo::data::AlarmCondition& value) {
            return setNewDefaultValue(value.asString());
        }


        typedef OverwriteElement OVERWRITE_ELEMENT;


    } // namespace data
} // namespace karabo
#endif
