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
    // having more than a thousand scans at once is pointless
    while(i<1000) {
        if(scanConf->HasSection(std::to_string(i))) {
            scanConf->SetSection(std::to_string(i));
            bool nested     = scanConf->Get("nested",false) && m_allow_nested_scan;
            double start    = scanConf->Get("start",std::numeric_limits<double>::min());
            double step     = scanConf->Get("step",std::numeric_limits<double>::min());
            double stop     = scanConf->Get("stop",std::numeric_limits<double>::min());
            double defaultV = scanConf->Get("default",start);
            int scanParallelTo = scanConf->Get("scanParallelTo", i);
            std::string param    = scanConf->Get("parameter","wrongPara");
            std::string name     = scanConf->Get("name","wrongPara");
            std::string Counter  = scanConf->Get("eventCounter","wrongPara");

            if(!m_scan_is_time_based && Counter == "wrongPara")
                return scanError("To run a scan based on a number of events, \"eventCounter\" needs to be specified in section"+std::to_string(i));
            if(name == "wrongPara" || param == "wrongPara"
               || (std::numeric_limits<double>::epsilon()>std::abs(start - std::numeric_limits<double>::min()))
               || (std::numeric_limits<double>::epsilon()>std::abs(stop - std::numeric_limits<double>::min()))
               || (std::numeric_limits<double>::epsilon()>std::abs(step - std::numeric_limits<double>::min())))
                return scanError("Scan section "+std::to_string(i)+" is incomplete -> Please check");

            defaultConf->SetSection("");
            if(!defaultConf->HasSection(name))
                return scanError("Scan section "+std::to_string(i)+":"+name+" is not existing in default configuration");
            defaultConf->SetSection(name);
            if(!defaultConf->Has(param))
                return scanError("Scan parameter "+std::to_string(i)+":"+param+" is not defined in default configuration");

            sec.push_back(ScanSection(start,stop, step, name, param, Counter, defaultV,i,nested, scanParallelTo));

            defaultConf->SetSection("");
            defaultConf->SetSection(name);
            defaultConf->SetString(param,std::to_string(defaultV));
        }
        ++i;
    }
    EUDAQ_INFO("Found "+std::to_string(sec.size())+" scans");
    defaultConf->SetSection("");

    createConfigs(0, defaultConf,sec);
    if(!m_repeatScans)
        EUDAQ_INFO("Scan with "+std::to_string(m_config_files.size())+" steps initialized");
    else
        EUDAQ_INFO("LOOPING Scan with "+std::to_string(m_config_files.size())+" steps initialized");
    m_n_steps = m_config_files.size();
    return true;
}

//void Scan::createConfigs(unsigned condition, eudaq::ConfigurationSP conf,std::vector<ScanSection> sec) {
//    if(condition == sec.size())
//        return;
//    auto confsBefore = m_config_files.size();

//    // We can scan in both directions:
//    bool decreasing = (sec.at(condition).start > sec.at(condition).stop);
//    auto value = sec.at(condition).start;
//    auto steps = std::abs(sec.at(condition).start-sec.at(condition).stop)/sec.at(condition).step;
//    EUDAQ_INFO( "Condition " + std::to_string(condition));
//    EUDAQ_INFO( "confsBefore " + std::to_string(confsBefore));
//    for(int nStep=0; nStep<steps; ++nStep)
//    {
//        // if the scan is nested, all other data points from before need to be
//        // redone with each point of current scan
//        if(sec.at(condition).nested) {
//            for(unsigned i =0; i < confsBefore;++i) {
//                eudaq::ConfigurationSP tmpConf = eudaq::Configuration::MakeUniqueReadFile(m_config_files.at(i));
//                tmpConf->SetSection(sec.at(condition).name);
//                tmpConf->Set(sec.at(condition).parameter, value);
//                storeConfigFile(tmpConf);
//                addSection(sec.at(condition));
//            }
//        } else {
//            conf->SetSection(sec.at(condition).name);
//            conf->Set(sec.at(condition).parameter, value);
//            storeConfigFile(conf);
//            EUDAQ_DEBUG(sec.at(condition).name+":"+sec.at(condition).parameter+":"+std::to_string(value));
//            addSection(sec.at(condition));
//        }
//        value+= (decreasing? -1: 1) *sec.at(condition).step;
//    }
//    conf->SetSection(sec.at(condition).name);
//    conf->Set(sec.at(condition).parameter, sec.at(condition).defaultV);
//    EUDAQ_DEBUG(sec.at(condition).name+":"+sec.at(condition).parameter+":"+std::to_string(value));
//    createConfigs(condition+1,conf,sec);
//}

void Scan::createConfigs(unsigned condition, eudaq::ConfigurationSP conf,std::vector<ScanSection> sec) {
    if(condition == sec.size())
        return;
    auto confsBefore = m_config_files.size();
    //std::vector<ScanSection>
    std::vector<double> value;
    std::vector<double> step;
    auto nSteps = std::abs(sec.at(condition).start-sec.at(condition).stop)/sec.at(condition).step;
    std::vector<bool> decreasing;
    std::vector<std::string> name;
    for (unsigned secCounter = 0; secCounter < sec.size(); secCounter++){ // loop through all later scan parameters and see if any of them have the same scanParallelTo
        if (sec.at(secCounter).scanParallelTo == condition){
            decreasing.push_back((sec.at(secCounter).start > sec.at(secCounter).stop));
            value.push_back(sec.at(secCounter).start);
            step.push_back(sec.at(secCounter).step);
        }
    }
    if (value.size() == 0){ // nothing to do
        return;
    }

    for(int nStep=0; nStep<nSteps; ++nStep)
    {
        // if the scan is nested, all other data points from before need to be
        // redone with each point of current
        if(sec.at(condition).nested) {
            for(unsigned i =0; i < confsBefore;++i) {
                eudaq::ConfigurationSP tmpConf = eudaq::Configuration::MakeUniqueReadFile(m_config_files.at(i));
                for (unsigned int param = 0; param < value.size(); param++){
                    tmpConf->SetSection(sec.at(param).name);
                    tmpConf->Set(sec.at(param).parameter, value.at(param));
                    addSection(sec.at(param));
                    value.at(param) += (decreasing.at(param)? -1: 1) *sec.at(param).step;
                }
                storeConfigFile(tmpConf);
            }
        }
        else {
            for (unsigned int param = 0; param < value.size(); param++){
                conf->SetSection(sec.at(param).name);
                conf->Set(sec.at(param).parameter, value.at(param));
                EUDAQ_DEBUG(sec.at(param).name+":"+sec.at(param).parameter+":"+std::to_string(value.at(param)));
                addSection(sec.at(param));
                value.at(param) += (decreasing.at(param)? -1: 1) *sec.at(param).step;
            }
        }
        storeConfigFile(conf);
    }
//    conf->SetSection(sec.at(condition).name);
//    conf->Set(sec.at(condition).parameter, sec.at(condition).defaultV);
    createConfigs(condition+1,conf,sec);
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
