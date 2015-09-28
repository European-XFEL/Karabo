/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 25, 2011, 5:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_STATISTICS_HH
#define	KARABO_XIP_STATISTICS_HH

#include <iostream>

namespace karabo {
    namespace xip {

        class Statistics {

            template <class T>
            class CpuImage;

            template <class T>
            friend class CpuImage;


            double m_min, m_max, m_mean, m_var, m_xMin, m_yMin, m_zMin, m_xMax, m_yMax, m_zMax;

        public:

            Statistics() {
            }

            Statistics(double min, double max, double mean, double sdev,
                       double xMin, double yMin, double zMin, double xMax, double yMax, double zMax)
            : m_min(min), m_max(max), m_mean(mean), m_var(sdev),
            m_xMin(xMin), m_yMin(yMin), m_zMin(zMin), m_xMax(xMax), m_yMax(yMax), m_zMax(zMax) {
            }

        public:

            inline const double getMin() const {
                return m_min;
            }

            inline const double getMax() const {
                return m_max;
            }

            inline const double getMean() const {
                return m_mean;
            }

            inline const double getVariance() const {
                return m_var;
            }

            inline const int getXmin() const {
                return m_xMin;
            }

            inline const int getYmin() const {
                return m_yMin;
            }

            inline const int getZmin() const {
                return m_zMin;
            }

            inline const int getXmax() const {
                return m_xMax;
            }

            inline const int getYmax() const {
                return m_yMax;
            }

            inline const int getZmax() const {
                return m_zMax;
            }

            inline void print() {
                std::cout << "Statistics:" << std::endl;
                std::cout << "  min = " << m_min << "  max = " << m_max << "  mean = " << m_mean << "  sdev = " << std::sqrt(m_var) << std::endl;
                std::cout << "  coords(min) = (" << m_xMin << ", " << m_yMin << ", " << m_zMin << ")" << std::endl;
                std::cout << "  coords(max) = (" << m_xMax << ", " << m_yMax << ", " << m_zMax << ")" << std::endl;
            }

        };

    }
}



#endif

