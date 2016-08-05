/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 23, 2011, 11:12 AM
 *
 * Major re-design on February 1, 2013, 1:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_OVERWRITEELEMENT_HH
#define	KARABO_UTIL_OVERWRITEELEMENT_HH

#include "Schema.hh"


namespace karabo {
    namespace util {

        class OverwriteElement {

        public:

            /**
             This class allows to define restrictions for the OVERWRITE_ELEMENT, i.e. which
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

                    Restriction() {
                    };

                    Restriction(const std::string& name_, Hash::Pointer restrictionPtr, const bool def) : name(name_), m_restrictionsPtr(restrictionPtr) {
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

                Restrictions() : m_rest(new Hash), alias("alias", m_rest, false),
                    displayedName("displayedName", m_rest, false),
                    description("description", m_rest, false),
                    tag("tag", m_rest, false),
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
                    stateOptions("stateOptions", m_rest, true), //true by default as all elements but state are restricted
                    allowedStates("allowedStates", m_rest, false),
                    observerAccess("observerAccess", m_rest, false),
                    userAccess("userAccess", m_rest, false),
                    operatorAccess("operatorAccess", m_rest, false),
                    expertAccess("expertAccess", m_rest, false),
                    adminAccess("adminAccess", m_rest, false),
                    unit("unit", m_rest, false),
                    metricPrefix("metricPrefix", m_rest, false),
                    overwriteRestrictions("overWriteRestrictions", m_rest, false) {

                }

                virtual ~Restrictions() {
                }

                /**
                 * Assigns from a vector<bool> indicating restrictions. Order of entries is in declaration order
                 * of restrictions.
                 * @param attrs
                 */
                void assignFromAttrVector(const std::vector<bool>& attrs) {
                    if (attrs.size() != m_rest->size()) {
                        throw KARABO_PARAMETER_EXCEPTION("Overwrite restrictions cannot be created from the passed attribute");
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

            OverwriteElement& setNewDisplayedName(const std::string& name);

            OverwriteElement& setNewDescription(const std::string& description);

            template <class AliasType>
            OverwriteElement& setNewAlias(const AliasType& alias) {
                checkIfRestrictionApplies(m_restrictions.alias);
                if (m_node) m_node->setAttribute<AliasType > (KARABO_SCHEMA_ALIAS, alias);
                return *this;
            }

            template <class TagType>
            OverwriteElement& setNewTag(const TagType& tag) {
                checkIfRestrictionApplies(m_restrictions.tag);
                if (m_node) m_node->setAttribute<TagType > ("tag", tag);
                return *this;
            }

            OverwriteElement& setNewAssignmentMandatory();

            OverwriteElement& setNewAssignmentOptional();

            OverwriteElement& setNewAssignmentInternal();

            OverwriteElement& setNowInit();

            OverwriteElement& setNowReconfigurable();

            OverwriteElement& setNowReadOnly();

            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.defaultValue);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMinInc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minInc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_INC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMaxInc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxInc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_INC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMinExc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minExc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_EXC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMaxExc(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxExc);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_EXC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMin(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.min);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMax(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.max);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMinSize(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.minSize);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMaxSize(const ValueType& value) {
                checkIfRestrictionApplies(m_restrictions.maxSize);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            OverwriteElement& setNewOptions(const std::string& opts, const std::string& sep = " ,;");
            OverwriteElement& setNewOptions(const std::vector<karabo::util::State>& opts);
            OverwriteElement& setNewOptions(const karabo::util::State& s1);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7);
            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7, const karabo::util::State& s8);
            OverwriteElement& setNewOptions(const std::vector<std::string>& opts);
            OverwriteElement& setNewAllowedStates(const std::vector<karabo::util::State>& states);
            //overloads for up to six states

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5);
            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6);
            OverwriteElement& setNowObserverAccess();

            OverwriteElement& setNowUserAccess();

            OverwriteElement& setNowOperatorAccess();

            OverwriteElement& setNowExpertAccess();

            OverwriteElement& setNowAdminAccess();

            OverwriteElement& setNewUnit(const UnitType& unit);

            OverwriteElement& setNewMetricPrefix(const MetricPrefixType& metricPrefix);

            /**
             * Adds new restrictions to the element by merging with existing restrictions
             * @param restrictions, contains the new set of restrictions as determined after merging with existing ones
             * @return
             */
            OverwriteElement& setNewOverwriteRestrictions(OverwriteElement::Restrictions & restrictions);

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
            Restrictions m_restrictions;

        };

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::State>(const karabo::util::State& value) {
            return setNewDefaultValue(toString(value));
        }

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::AlarmCondition>(const karabo::util::AlarmCondition& value) {
            return setNewDefaultValue(value.asString());
        }


        typedef OverwriteElement OVERWRITE_ELEMENT;


    }
}
#endif
