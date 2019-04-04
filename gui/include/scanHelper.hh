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
 * Additionally, the configs to be used are created
 *
 */
class Scan{
public:
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

    // setup Scan
    bool setupScan(string globalConfFile, string scanConfFile);
    // reset everything
    void reset();
    // get parameters
    int currentStep() const {return m_current_step;}
    int nSteps() const {return m_n_steps;}
    int timePerStep() const {return m_time_per_step;}
    int eventsPerStep() const {return m_events_per_step;}
    string nextConfig();
    bool scanIsTimeBased() const {return m_scan_is_time_based;}
    string currentCountingComponent() const{
        return m_events_counting_component.at((currentStep()-1)%m_events_counting_component.size());}
private:
    bool readGlobal(eudaq::ConfigurationSP conf);
    //void addConfig(string file) {m_config_files.push_back(file);}
    void setNested(bool n = false){m_allow_nested_scan = n;}
    bool ERROR(string msg) {    EUDAQ_ERROR(msg); return false;}
    bool storeConfigFile(eudaq::ConfigurationSP conf);
    void createConfigs(int condition, eudaq::ConfigurationSP conf, vector<ScanSection> sec);
    void add(ScanSection s);

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
