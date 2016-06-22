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

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace karabo {
    namespace util {
 
        
        class RollingWindowStatistics{
            
            unsigned long long m_evalInterval;
            unsigned long long m_nvals;
            double m_s, m_s2, m_meanEstimate;
            std::vector<double> m_vals;
            
            mutable boost::shared_mutex m_updateMutex;
            
            
            
        public:
            
                typedef boost::shared_ptr<RollingWindowStatistics> Pointer;
                typedef boost::shared_ptr<const RollingWindowStatistics> ConstPointer;
                
                RollingWindowStatistics(unsigned int evalInterval);

                virtual ~RollingWindowStatistics();
                
                void update(double v);
                
                
                double getRollingWindowVariance() const;
                
                double getRollingWindowMean() const;
                
        private:
            
                RollingWindowStatistics(const RollingWindowStatistics & other); //copy is protected
                RollingWindowStatistics();
                
                void updateEstimate();
        };
        
       
        
    }

}

#endif	/* STATISTICALEVALUATORS_HH */

