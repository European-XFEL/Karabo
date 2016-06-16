#include "StatisticalEvaluators.hh"

namespace karabo {
    namespace util {
        
        #define DEFAULT_EVAL_INTERVAL 100
        
        
        RollingWindowStatistics::RollingWindowStatistics(const unsigned int & evalInterval):
                m_evalInterval(evalInterval), m_s(0.), m_s2(0.), m_nvals(0),
                m_vals(evalInterval, 0){};
                
        RollingWindowStatistics::~RollingWindowStatistics(){
            
        }

        long double RollingWindowStatistics::getRollingWindowVariance() const {
            boost::shared_lock<boost::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return (long double)1./(n*(n-1))*(n*m_s2 - m_s*m_s);
        }
                
        long double RollingWindowStatistics::getRollingWindowMean() const {
            boost::shared_lock<boost::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return m_s/n;
        }
        
        // note that this is not the most numerically stable algorithms out there
        // but should suffice in the rolling window sizes allowed
        void RollingWindowStatistics::update(const double & v){
            long double vc;
            boost::unique_lock<boost::shared_mutex> lock(m_updateMutex);
            vc = m_vals[m_nvals % m_evalInterval];
            m_vals[m_nvals % m_evalInterval] = v;
            m_s = m_s + v - vc;
            m_s2 = m_s2 + (long double)v*v - vc*vc;
            m_nvals++;
                    
        }
        
        

    }
}
