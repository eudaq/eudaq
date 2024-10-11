#include "scanHelper.hh"

#include <cmath>


inline bool file_exists(std::string name) {
    std::ifstream f(name.c_str());
    return f.good();
}
bool Scan::setupScan(std::string globalConfFile, std::string scanConfFile) {
    //check if the files exist;
    if(!file_exists(globalConfFile))
        return scanError("Default config file not existing:  "+globalConfFile);
    if(!file_exists(scanConfFile))
        return scanError("Scan config file not existing:  "+scanConfFile);
    // create a configuration
    eudaq::ConfigurationSP defaultConf = eudaq::Configuration::MakeUniqueReadFile(globalConfFile);
    // read the scan config
    eudaq::ConfigurationSP scanConf = eudaq::Configuration::MakeUniqueReadFile(scanConfFile);
    if(!scanConf->HasSection("global"))
        return scanError("No global settings given in "+scanConfFile);

    if(!readGlobal(scanConf))
        return false;

    int i = 0 ;
    std::vector<ScanSection> sec;
    std::unordered_map<int, std::vector<ScanSection>> mapSec;
    // having more than a thousand scans at once is pointless
    int parallel = 0;
    while(i<1000) {
        if(scanConf->HasSection(std::to_string(i))) {
            scanConf->SetSection(std::to_string(i));
            bool nested     = scanConf->Get("nested",false) && m_allow_nested_scan;
            double start    = scanConf->Get("start",std::numeric_limits<double>::min());
            double step     = scanConf->Get("step",std::numeric_limits<double>::min());
            double stop     = scanConf->Get("stop",std::numeric_limits<double>::min());
            double defaultV = scanConf->Get("default",start);
            int scanParallelTo = scanConf->Get("scanParallelTo", parallel);
            std::string param    = scanConf->Get("parameter","wrongPara");
            std::string name     = scanConf->Get("name","wrongPara");
            std::string Counter  = scanConf->Get("eventCounter","wrongPara");
            EUDAQ_INFO("Checking condition with start "+std::to_string(std::numeric_limits<double>::epsilon()>std::abs(start - std::numeric_limits<double>::min())));
            EUDAQ_INFO("Checking condition with start epsilon "+std::to_string(std::numeric_limits<double>::epsilon()));
            EUDAQ_INFO("Checking condition with start other "+std::to_string(std::abs(start - std::numeric_limits<double>::min())));
            EUDAQ_INFO("Checking condition with start "+std::to_string(std::abs(start - std::numeric_limits<double>::min()) - std::numeric_limits<double>::epsilon()));
            if(!m_scan_is_time_based && Counter == "wrongPara")
                return scanError("To run a scan based on a number of events, \"eventCounter\" needs to be specified in section"+std::to_string(i));
            if(name == "wrongPara" || param == "wrongPara"
               || (std::numeric_limits<double>::epsilon()>std::abs(step - std::numeric_limits<double>::min())))
                return scanError("Scan section "+std::to_string(i)+" is incomplete -> Please check");

            defaultConf->SetSection("");
            if(!defaultConf->HasSection(name))
                return scanError("Scan section "+std::to_string(i)+":"+name+" is not existing in default configuration");
            defaultConf->SetSection(name);
            if(!defaultConf->Has(param))
                return scanError("Scan parameter "+std::to_string(i)+":"+param+" is not defined in default configuration");

            sec.push_back(ScanSection(start,stop, step, name, param, Counter, defaultV,i,nested, scanParallelTo));
            EUDAQ_INFO("Found "+std::to_string(scanParallelTo));
            mapSec[scanParallelTo].push_back(ScanSection(start,stop, step, name, param, Counter, defaultV,i,nested, scanParallelTo));

            defaultConf->SetSection("");
            defaultConf->SetSection(name);
            defaultConf->SetString(param,std::to_string(defaultV));
            parallel++;
        }
        ++i;
    }
    for (unsigned int mapIndex = 0; mapIndex < mapSec.size(); mapIndex++){
        if (mapSec[mapIndex].size() > 1){
            unsigned int oldSteps;
            for (unsigned int vecIndex = 0; vecIndex < mapSec[mapIndex].size(); vecIndex++){
                unsigned int steps = std::abs((mapSec[mapIndex].at(vecIndex).start-mapSec[mapIndex].at(vecIndex).stop)/mapSec[mapIndex].at(vecIndex).step);
                if (vecIndex > 0) {
                    if (steps != oldSteps) return scanError("Parallel scan parameters require the same number of steps! Found "+std::to_string(steps) + " versus " +std::to_string(oldSteps) +" steps!");
                }
                oldSteps = steps;
            }
        }
    }
    EUDAQ_INFO("Found "+std::to_string(sec.size())+" scans");
    defaultConf->SetSection("");

    createConfigsMulti(0, defaultConf, mapSec);
    if(!m_repeatScans)
        EUDAQ_INFO("Scan with "+std::to_string(m_config_files.size())+" steps initialized");
    else
        EUDAQ_INFO("LOOPING Scan with "+std::to_string(m_config_files.size())+" steps initialized");
    m_n_steps = m_config_files.size();
    return true;
}

void Scan::createConfigsMulti(unsigned condition, eudaq::ConfigurationSP conf, std::unordered_map<int, std::vector<ScanSection>> mapSec) {
    EUDAQ_INFO("Map size = " + std::to_string(mapSec.size()));
    if(condition == mapSec.size())
        return;
    auto confsBefore = m_config_files.size();
    std::vector<double> value;
    std::vector<double> step;
    auto nSteps = std::abs((mapSec[condition].at(0).start-mapSec[condition].at(0).stop)/mapSec[condition].at(0).step);
    std::vector<bool> decreasing;
    std::vector<std::string> name;
    for (unsigned secCounter = 0; secCounter < mapSec[condition].size(); secCounter++){ // loop through all parallel scan parameters
        EUDAQ_DEBUG("Start parameter of section is " +std::to_string(mapSec[condition].at(secCounter).start ));
        EUDAQ_DEBUG("Stop parameter of section is " +std::to_string(mapSec[condition].at(secCounter).stop ));
        decreasing.push_back((mapSec[condition].at(secCounter).start > mapSec[condition].at(secCounter).stop));
        value.push_back(mapSec[condition].at(secCounter).start);
        step.push_back(mapSec[condition].at(secCounter).step);
    }
    if (value.size() == 0){ // safety exit in case something went wrong.
        return;
    }
    EUDAQ_INFO("nested = " + std::to_string(mapSec[condition].at(0).nested) + " for " + mapSec[condition].at(0).name);
    for(int nStep=0; nStep<nSteps; ++nStep)
    {
        // if the scan is nested, all other data points from before need to be
        // redone with each point of current
        if(mapSec[condition].at(0).nested) {
            for(unsigned i =0; i < confsBefore;++i) {
                eudaq::ConfigurationSP tmpConf = eudaq::Configuration::MakeUniqueReadFile(m_config_files.at(i));
                for (unsigned int param = 0 ; param < value.size(); param++){
                    tmpConf->SetSection(mapSec[condition].at(param).name);
                    tmpConf->Set(mapSec[condition].at(param).parameter, value.at(param));
                    EUDAQ_DEBUG("Nested "+mapSec[condition].at(param).name+":"+mapSec[condition].at(param).parameter+":"+std::to_string(value.at(param)));
                    addSection(mapSec[condition].at(param));
                }
                storeConfigFile(tmpConf);
            }
            for (unsigned int param = 0; param < value.size(); param++){
                value.at(param) += (decreasing.at(param)? -1: 1) *std::abs(mapSec[condition].at(param).step);
            }
        }
        else {
            for (unsigned int param = 0; param < value.size(); param++){
                conf->SetSection(mapSec[condition].at(param).name);
                conf->Set(mapSec[condition].at(param).parameter, value.at(param));
                EUDAQ_DEBUG("Non-Nested "+mapSec[condition].at(param).name+":"+mapSec[condition].at(param).parameter+":"+std::to_string(value.at(param)));
                addSection(mapSec[condition].at(param));
                value.at(param) += (decreasing.at(param)? -1: 1) *std::abs(mapSec[condition].at(param).step);
            }
            storeConfigFile(conf);
        }
    }
    for (unsigned int param = 0; param < value.size(); param++){
        conf->SetSection(mapSec[condition].at(param).name);
        conf->Set(mapSec[condition].at(param).parameter, mapSec[condition].at(param).defaultV);
    }
    EUDAQ_DEBUG("Moving to next parameter");
    createConfigsMulti(condition+1,conf,mapSec);
}


bool Scan::storeConfigFile(eudaq::ConfigurationSP conf) {
    std::string filename = m_config_file_prefix+"_"+std::to_string(m_config_files.size())+".conf";
    EUDAQ_INFO(filename);
    std::filebuf fb;
    fb.open (filename,std::ios::out);
    std::ostream os(&fb);
    conf->Save(os);
    fb.close();
    m_config_files.push_back(filename);
    return file_exists(filename);
}

void Scan::addSection(ScanSection s) {
    m_scanned_parameter.push_back(s.parameter);
    m_scanned_producer.push_back(s.name);
    m_events_counting_component.push_back(s.eventCounter);
}

void Scan::reset() {
    m_config_file_prefix = "";
    m_config_files.clear();
    m_allow_nested_scan = false;
    m_scan_is_time_based = true;
    m_repeatScans = false;
    m_time_per_step = 0;
    m_events_per_step = 0;
    m_steps_per_scan.clear();
    m_n_steps = 0;
    m_scanned_parameter.clear();
    m_scanned_producer.clear();
    m_events_counting_component.clear();
    m_current_step = 0;
}

std::string Scan::nextConfig() {
    m_current_step++;
    if(m_current_step>m_config_files.size() && !m_repeatScans)
        return "finished";
    else return m_config_files.at((m_current_step-1)%(m_config_files.size()));
}

std::string Scan::currentCountingComponent() const {
    return m_events_counting_component.at((currentStep()-1)%m_events_counting_component.size());
}

bool Scan::readGlobal(eudaq::ConfigurationSP conf) {
    conf->SetSection("global");
    m_allow_nested_scan   = conf->Get("allowNested", false);
    m_repeatScans         = conf->Get("repeatScans", false);
    m_config_file_prefix  = conf->Get("configPrefix","default");
    m_scan_is_time_based  = conf->Get("timeBasedScan",true);
    m_time_per_step       = conf->Get("timePerStep",-1);
    m_events_per_step     = conf->Get("nEventsPerStep",-1);
    if((!m_scan_is_time_based && m_events_per_step == -1) || (m_scan_is_time_based && m_time_per_step == -1))
        return scanError("Global scan configuration wrong: per default time based scans -> "
                     "timePerStep needs to be defined/if not time based number of events must be given!");
    return true;
}
