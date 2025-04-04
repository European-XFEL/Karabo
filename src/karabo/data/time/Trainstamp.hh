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
 * File:   TrainStamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_DATA_TIME_TRAINSTAMP_HH
#define KARABO_DATA_TIME_TRAINSTAMP_HH

#include "karabo/data/types/Hash.hh"


namespace karabo {
    namespace data {

        /**
         * This class expresses a time point and holds it in form of one unsigned 64bit value.
         * The value is the European XFEL trainId which clocks in regular interval (initially 10Hz)
         */
        class Trainstamp {
            unsigned long long m_trainId;

           public:
            /**
             * Default constructor creates invalid trainId (=0)
             */
            Trainstamp();


            /**
             * Constructor from trainId
             */
            Trainstamp(const unsigned long long trainId);

            inline const unsigned long long& getTrainId() const {
                return m_trainId;
            }

            static bool hashAttributesContainTimeInformation(const Hash::Attributes& attributes);


            /**
             * Creates an TrainStamp from a Hash attribute
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return TrainStamp object
             */
            static Trainstamp fromHashAttributes(const Hash::Attributes& attributes);

            /**
             * Formats as Hash attributes
             * @param attributes container to which the time point information is added
             */
            void toHashAttributes(Hash::Attributes& attributes) const;


            virtual ~Trainstamp();

            friend bool operator==(const Trainstamp& lhs, const Trainstamp& rhs);

            friend bool operator!=(const Trainstamp& lhs, const Trainstamp& rhs);

           private:
        };

        std::ostream& operator<<(std::ostream&, const Trainstamp& trainstamp);

    } // namespace data
} // namespace karabo

#endif
