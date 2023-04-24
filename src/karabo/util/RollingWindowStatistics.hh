/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   StatisticalEvaluators.hh
 * Author: haufs
 *
 * Created on June 13, 2016, 1:46 PM
 */

#ifndef KARABO_ROLLINGWINDOWSTATISTICS_HH
#define KARABO_ROLLINGWINDOWSTATISTICS_HH

#include <algorithm>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <vector>


namespace karabo {
    namespace util {

        class RollingWindowStatistics {
           public:
            typedef boost::shared_ptr<RollingWindowStatistics> Pointer;
            typedef boost::shared_ptr<const RollingWindowStatistics> ConstPointer;

            /**
             * A rolling window statistics evaluator
             * @param evalInterval: rolling window interval to evaluate for
             */
            RollingWindowStatistics(unsigned int evalInterval);

            virtual ~RollingWindowStatistics();

            /**
             * Updates evaluated value list. If the new calculated rolling mean deviates
             * from the currently used estimate of the mean by more than five sigma the
             * estimate is updated as well.
             * @param v
             */
            void update(double v);

            /**
             * Returns the rolling variance
             * @return
             */
            double getRollingWindowVariance() const;

            /**
             * Returns the rolling mean
             * @return
             */
            double getRollingWindowMean() const;


            /**
             * Return the interval for these statistics
             * @return
             */
            unsigned long long getInterval() const;

           protected:
            double m_meanEstimate; // protected mainly to allow for testing

           private:
            RollingWindowStatistics(const RollingWindowStatistics& other); // copy is protected
            RollingWindowStatistics();

            void updateEstimate(const double currentMean);

            unsigned long long m_evalInterval;
            unsigned long long m_nvals;
            double m_s, m_s2;
            std::vector<double> m_vals;

            mutable boost::shared_mutex m_updateMutex;
        };

    } // namespace util

} // namespace karabo

#endif /* STATISTICALEVALUATORS_HH */
