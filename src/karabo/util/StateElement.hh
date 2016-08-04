/*
 * File:   StateElement.h
 * Author: haufs
 *
 * Created on July 22, 2016, 5:34 PM
 */

#ifndef KARABO_SCHEMA_STATEELEMENT_H
#define	KARABO_SCHEMA_STATEELEMENT_H


#include "State.hh"
#include "Schema.hh"

namespace karabo {
    namespace util {

        /**
         * The StateElement represents a leaf and needs to be of type State
         */

        class StateElement : public GenericElement<StateElement> {

        public:

            StateElement(Schema& expected) : GenericElement<StateElement>(expected) {

            }

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param one to eight States or a vector of States
             * @return reference to the StateElement
             */
            StateElement& options(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return options(std::vector<karabo::util::State>(arr, arr + 1));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return options(std::vector<karabo::util::State>(arr, arr + 2));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return options(std::vector<karabo::util::State>(arr, arr + 3));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return options(std::vector<karabo::util::State>(arr, arr + 4));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return options(std::vector<karabo::util::State>(arr, arr + 5));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return options(std::vector<karabo::util::State>(arr, arr + 6));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
                return options(std::vector<karabo::util::State>(arr, arr + 7));
            }

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7, const karabo::util::State& s8) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
                return options(std::vector<karabo::util::State>(arr, arr + 8));
            }

            StateElement& options(const std::vector<karabo::util::State>& opts) {
                return options(toString(opts), ",");
            }

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            StateElement& initialValue(const karabo::util::State& s) {
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, s.name());
                return *this;
            }


        protected:

            void beforeAddition() {
                /**
                 * Schema is a read-only, leaf property
                 */
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, Schema::STATE);
                this->m_node->template setAttribute<std::string>(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);
                this->m_node->template setAttribute<bool>(KARABO_INDICATE_STATE_SET, true);

                //finally protect setting options etc to state element via overwrite
                OverwriteElement::Restrictions restrictions;
                restrictions.options = true;
                restrictions.minInc = true;
                restrictions.minExc = true;
                restrictions.maxInc = true;
                restrictions.maxExc = true;
                restrictions.readOnly = true;
                restrictions.reconfigurable = true;
                restrictions.displayedName = true;
                restrictions.overwriteRestrictions = true;
                restrictions.stateOptions = false; //set to true by default
                m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());

            }

        private:

            StateElement& options(const std::string& opts, const std::string& sep = " ,;") {
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            StateElement& options(const std::vector<std::string>& opts) {
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }



        };



        typedef StateElement STATE_ELEMENT;


    }
}


#endif	/* KARABO_SCHEMA_STATEELEMENT_H */

