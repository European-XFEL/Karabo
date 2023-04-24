/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   StateElement.h
 * Author: haufs
 *
 * Created on July 22, 2016, 5:34 PM
 */

#ifndef KARABO_SCHEMA_STATEELEMENT_H
#define KARABO_SCHEMA_STATEELEMENT_H

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        class Schema;
        class State;

        /**
         * @class StateElement
         * @brief The StateElement represents a leaf and needs to be of type State
         */
        class StateElement : public GenericElement<StateElement> {
           public:
            StateElement(Schema& expected);

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param one to eight States or a vector of States
             * @return reference to the StateElement
             */
            StateElement& options(const karabo::util::State& s1);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5, const karabo::util::State& s6);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5, const karabo::util::State& s6,
                                  const karabo::util::State& s7);

            StateElement& options(const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5, const karabo::util::State& s6,
                                  const karabo::util::State& s7, const karabo::util::State& s8);

            StateElement& options(const std::vector<karabo::util::State>& opts);

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            StateElement& initialValue(const karabo::util::State& s);

            StateElement& daqPolicy(const DAQPolicy& policy);

           protected:
            void beforeAddition();

           private:
            StateElement& options(const std::string& opts, const std::string& sep = " ,;");

            StateElement& options(const std::vector<std::string>& opts);
        };

        typedef StateElement STATE_ELEMENT;

    } // namespace util
} // namespace karabo


#endif /* KARABO_SCHEMA_STATEELEMENT_H */
