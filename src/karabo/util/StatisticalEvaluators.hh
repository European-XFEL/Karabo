/* 
 * File:   StatisticalEvaluators.hh
 * Author: haufs
 *
 * Created on June 13, 2016, 1:46 PM
 */

#ifndef STATISTICALEVALUATORS_HH
#define	STATISTICALEVALUATORS_HH

#include <vector>
#include <algorithm>

namespace karabo {
    namespace util {
 
        
        class RollingWindowStatistics{
            
            unsigned long long m_evalInterval;
            unsigned long long m_nvals;
            double m_s, m_s2;
            std::vector<double> m_vals;
            
            public:
                
                RollingWindowStatistics();
                
                RollingWindowStatistics(const unsigned int & evalInterval);
                
                virtual ~RollingWindowStatistics();
                
                void update(const double & v);
                
                double getRollingWindowVariance() const;
                
                double getRollingWindowMean() const;
                
                
        };
        
    }

}

#endif	/* STATISTICALEVALUATORS_HH */

