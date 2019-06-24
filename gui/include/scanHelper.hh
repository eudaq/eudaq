#ifndef EUDAQ_INCLUDED_GUI_ScanHelper
#define EUDAQ_INCLUDED_GUI_ScanHelper

#include <stdio.h>
#include <string>
#include <vector>
#include "eudaq/Config.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RunControl.hh"

using namespace std;

/**
 * @brief The Scan class
 * @abstract Store all parameters requried to perform a scan
 * Additionally, the configs to be used are created and stored
 *
 */
class Scan{
public:
    /**
     * @brief The ScanSection struct, which stores the information from one user section in a Scan Configuration
     */
    struct ScanSection{
        double start, step, stop, defaultV;
        int secIndex;
        bool nested;
        string name, parameter, eventCounter;

        ScanSection(double start, double stop, double step, string name,string parameter,string eventCounter, double defaultV, int idx, bool nested = false)
            : start(start), stop(stop), step(step), name(name), nested(nested), parameter(parameter), eventCounter(eventCounter), defaultV(defaultV), secIndex(idx)
        {}
    };
    Scan():m_allow_nested_scan(false), m_scan_is_time_based(true), m_time_per_step(0), m_repeatScans(false),
        m_events_per_step(-10), m_current_step(0), m_config_files(), m_steps_per_scan(), m_n_steps(0),
        m_scanned_parameter(), m_scanned_producer(), m_events_counting_component(), m_config_file_prefix("defaultScan")
    {
        reset();
    }

    /**
     * @brief setupScan
     * @param globalConfFile = default configuration file
     * @param scanConfFile   = scan config file
     * @return true if the scan could be configured correctly, false else
     */
    bool setupScan(string globalConfFile, string scanConfFile);
    /**
     * @brief Reset all local parameters, created files are not deleted
     */
    void reset();
    /**
     * @brief currentStep
     * @return Current step in scan. Can be larger than the number of steps if a scan is started in an lopp
     */
    int currentStep()       const {return m_current_step;}
    /**
     * @brief nSteps
     * @return number of steps in scan
     */
    int nSteps()            const {return m_n_steps;}

    /**
     * @brief timePerStep
     * @return time per Step
     */
    int timePerStep()       const {return m_time_per_step;}
    /**
     * @brief eventsPerStep
     * @return events per Step
     */
    int eventsPerStep()     const {return m_events_per_step;}

    /**
     * @brief nextConfig
     * @return the configuration filename for the next step in the scan or 'finished', if the scan is completed
     */
    string nextConfig();
    /**
     * @brief scanIsTimeBased
     * @return true if the steps are selected based on runtime
     */
    bool scanIsTimeBased()  const {return m_scan_is_time_based;}
    /**
     * @brief currentCountingComponent
     * @return the connection which provides the number of events required in a scan step
     */
    string currentCountingComponent() const;
private:

    /**
     * @brief ERROR message from EUDAQ_ERROR with return statement
     * @param msg
     * @return false
     */
    bool ERROR(string msg) {    std::cout << msg <<std::endl; EUDAQ_ERROR(msg); return false;}
    /**
     * @brief read the global config
     * @param conf
     * @return true if reading completed, false otherwise
     */
    bool readGlobal(eudaq::ConfigurationSP conf);

    /**
     * @brief Store configuration for a step
     * @param conf Configuration to be stored
     * @return True if file created
     */
    bool storeConfigFile(eudaq::ConfigurationSP conf);
    /**
     * @brief Creates the configuration files for each scan
     * @param condition Recursvice iterator over the scan secctions 'sec'
     * @param conf Default configuration
     * @param sec Scan section beeing read
     */
    void createConfigs(int condition, eudaq::ConfigurationSP conf, vector<ScanSection> sec);
    /**
     * @brief add additional information for each section
     * @param s Section read in
     */
    void addSection(ScanSection s);

    vector<string> m_config_files;
    bool m_allow_nested_scan = false;
    bool m_scan_is_time_based = true;
    bool m_repeatScans = false;
    int m_time_per_step = 0;
    int m_events_per_step = 0;
    vector<int> m_steps_per_scan;
    int m_n_steps = 0;
    vector<string> m_scanned_parameter;
    vector<string> m_scanned_producer;
    vector<string> m_events_counting_component;
    string m_config_file_prefix;
    int m_current_step = 0;


};




#endif // EUDAQ_INCLUDED_GUI_ScanHelper
