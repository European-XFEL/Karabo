#include "RollingWindowStatistics.hh"

namespace karabo {
    namespace util {
        
        
        
        
        RollingWindowStatistics::RollingWindowStatistics(unsigned int evalInterval):
                m_evalInterval(evalInterval), m_s(0.), m_s2(0.), m_nvals(0),
                m_meanEstimate(0.), m_vals(evalInterval, 0.){};
                
        RollingWindowStatistics::~RollingWindowStatistics(){
            
        }

        
        double RollingWindowStatistics::getRollingWindowVariance() const {
            boost::shared_lock<boost::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return (m_s2 - m_s*m_s/n)/(n-1);
        }
                
        
        double RollingWindowStatistics::getRollingWindowMean() const {
            boost::shared_lock<boost::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return m_s/n+m_meanEstimate;
        }
        
        
        void RollingWindowStatistics::update(double v){
            
            const double varBeforeUpdate = getRollingWindowVariance();
            boost::unique_lock<boost::shared_mutex> lock(m_updateMutex);
            if (m_nvals == 0) {
                m_meanEstimate = v;
            
            }
            const unsigned int index = m_nvals % m_evalInterval;
            const double vOldest = m_vals[index];
            m_vals[index] = v;
            const double diffCurrent = v - m_meanEstimate; 
            const double diffOldest = (m_nvals >=  m_evalInterval ? vOldest - m_meanEstimate:  0.); 
            m_s +=  (diffCurrent - diffOldest);
            m_s2 += (diffCurrent * diffCurrent - diffOldest * diffOldest);
            ++m_nvals;

            lock.unlock();
            const double currentMean = getRollingWindowMean();
            if((currentMean-m_meanEstimate)*(currentMean-m_meanEstimate)/varBeforeUpdate > 25 ) updateEstimate(currentMean); // we update if we are 5sigma off
               
        }
        
        
        void RollingWindowStatistics::updateEstimate(const double currentMean){
            //we need to go through all data in current estimate
            boost::mutex::scoped_lock<boost::shared_mutex> lock(m_updateMutex);
            m_s = 0.;
            m_s2 = 0.;
            m_meanEstimate = currentMean;
            const unsigned int n = std::min(m_evalInterval, m_nvals);
            for(unsigned int i = 0; i < n ; i++){
                const double v = m_vals[i];
                const double diff = v - m_meanEstimate; 
                m_s +=  diff;
                m_s2 += diff * diff;
            }
                       
        }
        
       
        

    }
}
