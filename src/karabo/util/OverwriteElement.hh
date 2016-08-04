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

                    Hash::Pointer m_restrictionsPtr;
                    std::string name;

                    Restriction() {
                    };

                    Restriction(const std::string& name_, Hash::Pointer restrictionPtr, const bool def) : name(name_), m_restrictionsPtr(restrictionPtr) {
                        m_restrictionsPtr->set(name, def);
                    };

                    Restriction& operator=(const bool rhs) {
                        m_restrictionsPtr->set(name, rhs);
                    }

                    bool operator==(const bool rhs) const {
                        return m_restrictionsPtr->get<bool>(name) == rhs;
                    }

                    bool operator!=(const bool rhs) const {
                        return m_restrictionsPtr->get<bool>(name) != rhs;
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
                    stateOptions("stateOptions", m_rest, true),
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

            OverwriteElement(Schema& expected) : m_schema(&expected), m_protectOptions(true) {
            }

            /**
             * Specify the key to be overwritten
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name) {


                boost::optional<Hash::Node&> node = m_schema->getParameterHash().find(name);
                if (node) { // exists
                    m_node = node.get_ptr();
                    if (node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                        m_restrictions.assignFromAttrVector(node->getAttribute < std::vector<bool> >(KARABO_OVERWRITE_RESTRICTIONS));
                    }
                } else {
                    // Could be, the parameter is assembled under different rules, we should silently ignore this then.
                    m_node = 0;
                    //throw KARABO_PARAMETER_EXCEPTION("Key \"" + name + "\" was not set before, thus can not be overwritten.");
                }
                return *this;
            }

            OverwriteElement& setNewDisplayedName(const std::string& name) {
                checkIfRestrictionApplies(m_restrictions.displayedName);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DISPLAYED_NAME, name);
                return *this;
            }

            OverwriteElement& setNewDescription(const std::string& description) {
                checkIfRestrictionApplies(m_restrictions.description);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DESCRIPTION, description);
                return *this;
            }

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

            OverwriteElement& setNewAssignmentMandatory() {
                checkIfRestrictionApplies(m_restrictions.assignmentMandatory);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentOptional() {
                checkIfRestrictionApplies(m_restrictions.assignmentOptional);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentInternal() {
                checkIfRestrictionApplies(m_restrictions.assignmentInternal);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return *this;
            }

            OverwriteElement& setNowInit() {
                checkIfRestrictionApplies(m_restrictions.init);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            OverwriteElement& setNowReconfigurable() {
                checkIfRestrictionApplies(m_restrictions.reconfigurable);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }

            OverwriteElement& setNowReadOnly() {
                checkIfRestrictionApplies(m_restrictions.readOnly);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                return *this;
            }

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

            OverwriteElement& setNewOptions(const std::string& opts, const std::string& sep = " ,;") {
                if (m_protectOptions) checkIfRestrictionApplies(m_restrictions.options); //only protect if set from outside.
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            OverwriteElement& setNewOptions(const std::vector<karabo::util::State>& opts) {
                checkIfRestrictionApplies(m_restrictions.stateOptions);
                m_protectOptions = false;
                return setNewOptions(toString(opts), ",");
                m_protectOptions = true;
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 1));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 2));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 3));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 4));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 5));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 6));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 7));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7, const karabo::util::State& s8) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 8));
            }

            OverwriteElement& setNewOptions(const std::vector<std::string>& opts) {
                checkIfRestrictionApplies(m_restrictions.options);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }

            OverwriteElement& setNewAllowedStates(const std::vector<karabo::util::State>& states) {
                checkIfRestrictionApplies(m_restrictions.allowedStates);
                const std::string stateList = karabo::util::toString(states);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector > (stateList, ","));

                return *this;
            }

            //overloads for up to six states

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 1));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 2));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 3));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 4));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 5));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 6));
            }

            OverwriteElement& setNowObserverAccess() {
                checkIfRestrictionApplies(m_restrictions.observerAccess);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
                return *this;
            }

            OverwriteElement& setNowUserAccess() {
                checkIfRestrictionApplies(m_restrictions.userAccess);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::USER);
                return *this;
            }

            OverwriteElement& setNowOperatorAccess() {
                checkIfRestrictionApplies(m_restrictions.operatorAccess);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OPERATOR);
                return *this;
            }

            OverwriteElement& setNowExpertAccess() {
                checkIfRestrictionApplies(m_restrictions.expertAccess);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::EXPERT);
                return *this;
            }

            OverwriteElement& setNowAdminAccess() {
                checkIfRestrictionApplies(m_restrictions.adminAccess);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::ADMIN);
                return *this;
            }

            OverwriteElement& setNewUnit(const UnitType& unit) {
                checkIfRestrictionApplies(m_restrictions.unit);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, unit);
                std::pair<std::string, std::string> names = karabo::util::getUnit(unit);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
                return *this;
            }

            OverwriteElement& setNewMetricPrefix(const MetricPrefixType& metricPrefix) {
                checkIfRestrictionApplies(m_restrictions.metricPrefix);
                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix);
                std::pair<std::string, std::string> names = karabo::util::getMetricPrefix(metricPrefix);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
                return *this;
            }

            OverwriteElement& setNewOverwriteRestrictions(OverwriteElement::Restrictions & restrictions) {
                checkIfRestrictionApplies(m_restrictions.overwriteRestrictions);
                if (m_node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                    OverwriteElement::Restrictions existing;
                    existing.assignFromAttrVector(m_node->getAttribute < vector<bool> >(KARABO_OVERWRITE_RESTRICTIONS));
                    //now merge
                    restrictions.merge(existing);
                }
                m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
                return *this;
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            void commit() {
                // Does nothing, changes happened on existing node
            }

        private:

            /**
             * Throws an exemption if the restriction in question is set
             * @param restriction
             */
            void checkIfRestrictionApplies(const Restrictions::Restriction& restriction) const {
                if (restriction == true) {
                    const std::string& key = m_node->getKey();
                    const std::string& name = restriction.name;
                    const std::string& msg = "Element (" + key + ") does not allow overwriting attribute " + name + "!";
                    throw KARABO_LOGIC_EXCEPTION(msg);
                }
            }

            Schema* m_schema;
            Hash::Node* m_node;
            Restrictions m_restrictions;
            bool m_protectOptions;
        };

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::State>(const karabo::util::State& value) {
            return setNewDefaultValue(toString(value));
        }


        typedef OverwriteElement OVERWRITE_ELEMENT;


    }
}
#endif
