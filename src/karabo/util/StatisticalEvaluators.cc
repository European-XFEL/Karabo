#include "StatisticalEvaluators.hh"

namespace karabo {
    namespace util {
        
        #define DEFAULT_EVAL_INTERVAL 100
        
        RollingWindowStatistics::RollingWindowStatistics():
                m_evalInterval(DEFAULT_EVAL_INTERVAL), m_s(0.), m_s2(0.), m_nvals(0),
                m_vals(DEFAULT_EVAL_INTERVAL, 0){};
        
        RollingWindowStatistics::RollingWindowStatistics(const unsigned int & evalInterval):
                m_evalInterval(evalInterval), m_s(0.), m_s2(0.), m_nvals(0),
                m_vals(evalInterval, 0){};
        
        RollingWindowStatistics::~RollingWindowStatistics(){
            
        }

        double RollingWindowStatistics::getRollingWindowVariance() const {
                    unsigned long long n = std::min(m_evalInterval, m_nvals);
                    return 1./(n*(n-1))*(n*m_s2 - m_s*m_s);
                }
                
        double RollingWindowStatistics::getRollingWindowMean() const {
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return m_s/n;
        }
        
        // note that this is not the most numerically stable algorithms out there
        // but should suffice in the rolling window sizes allowed
        void RollingWindowStatistics::update(const double & v){
                    m_nvals++;
                    double vc;
                    vc = m_vals[(m_nvals % m_evalInterval) - 1];
                    m_vals[(m_nvals % m_evalInterval) - 1] = v;
                    m_s = m_s + v - vc;
                    m_s2 = m_s2 + v*v - vc*vc;
                    
        }
        
        

    }
}
