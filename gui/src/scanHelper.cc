#include "scanHelper.hh"

inline bool file_exists(string name) {
    ifstream f(name.c_str());
    return f.good();
}
bool Scan::setupScan(string globalConfFile, string scanConfFile) {
    //check if the files exist;
    if(!file_exists(globalConfFile))
        return ERROR("Default config file not existing:  "+globalConfFile);
    if(!file_exists(scanConfFile))
        return ERROR("Scan config file not existing:  "+scanConfFile);
    // create a configuration
    eudaq::ConfigurationSP defaultConf = eudaq::Configuration::MakeUniqueReadFile(globalConfFile);
    // read the scan config
    eudaq::ConfigurationSP scanConf = eudaq::Configuration::MakeUniqueReadFile(scanConfFile);
    if(!scanConf->HasSection("global"))
        return ERROR("No global settings given in "+scanConfFile);

    if(!readGlobal(scanConf))
        return false;
    int i = 0 ;
    vector<ScanSection> sec;
    // having more than a thousand scans at once is pointless
    while(i<1000) {
        if(scanConf->HasSection(to_string(i))) {
            scanConf->SetSection(to_string(i));
            bool nested     = scanConf->Get("nested",false) && m_allow_nested_scan;
            double start    = scanConf->Get("start",-123456789);
            double step     = scanConf->Get("step",-123456789);
            double stop     = scanConf->Get("stop",-123456789);
            double defaultV = scanConf->Get("default",start);
            string param    = scanConf->Get("parameter","wrongPara");
            string name     = scanConf->Get("name","wrongPara");
            string Counter  = scanConf->Get("eventCounter","wrongPara");
            // check if any parameter is wrong
            if(Counter == "wrongPara" || name == "wrongPara" || param == "wrongPara"
                    || start == -123456789 || stop == -123456789 || step == -123456789)
                return ERROR("Scan section "+to_string(i)+" is incomplete -> Please check");
            defaultConf->SetSection("");

            // check if the component is existing in the default config and the scanned parameter existing
            if(!defaultConf->HasSection(name))
                return ERROR("Scan section "+to_string(i)+":"+name+" is not in default config");
            defaultConf->SetSection(name);
            if(!defaultConf->Has(param))
                return ERROR("Scan section "+to_string(i)+":"+param+" is not in default config");

            sec.push_back(ScanSection(start,stop, step, name, param, Counter, defaultV,i,nested));
            // set the correct defaults
            defaultConf->SetSection("");
            defaultConf->SetSection(name);
            defaultConf->SetString(param,to_string(defaultV));
        }
        ++i;
    }
    EUDAQ_INFO("Found "+to_string(sec.size())+" scans");
    defaultConf->SetSection("");

    createConfigs(0, defaultConf,sec);
    if(!m_repeatScans)
        EUDAQ_INFO("Scan with "+to_string(m_config_files.size())+" steps initialized");
    else
        EUDAQ_INFO("LOOPING Scan with "+to_string(m_config_files.size())+" steps initialized");
    m_n_steps = m_config_files.size();
    return true;
}

void Scan::createConfigs(int condition, eudaq::ConfigurationSP conf,vector<ScanSection> sec) {
    if(condition == sec.size())
        return;
    int confsBefore = m_config_files.size();
    auto it = sec.at(condition).start;
    while(it<=sec.at(condition).stop) {
        // if the scan is nested, all other data points from before need to be
        // redone with each point of current scan
        if(sec.at(condition).nested) {
            for(int i =0; i < confsBefore;++i) {
                eudaq::ConfigurationSP tmpConf = eudaq::Configuration::MakeUniqueReadFile(m_config_files.at(i));
                tmpConf->SetSection(sec.at(condition).name);
                tmpConf->Set(sec.at(condition).parameter, it);
                storeConfigFile(tmpConf);
                addSection(sec.at(condition));
            }
        } else {
            conf->SetSection(sec.at(condition).name);
            conf->Set(sec.at(condition).parameter, it);
            storeConfigFile(conf);
            EUDAQ_DEBUG(sec.at(condition).name+":"+sec.at(condition).parameter+":"+to_string(it));
            addSection(sec.at(condition));
        }
        it+= sec.at(condition).step;
    }
    conf->SetSection(sec.at(condition).name);
    conf->Set(sec.at(condition).parameter, sec.at(condition).defaultV);
    EUDAQ_DEBUG(sec.at(condition).name+":"+sec.at(condition).parameter+":"+to_string(it));
    createConfigs(condition+1,conf,sec);
}

bool Scan::storeConfigFile(eudaq::ConfigurationSP conf) {
    string filename = m_config_file_prefix+"_"+to_string(m_config_files.size())+".conf";
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

string Scan::nextConfig() {
    m_current_step++;
    if(m_current_step>m_config_files.size() && !m_repeatScans)
        return "finished";
    else return m_config_files.at((m_current_step-1)%(m_config_files.size()));
}

string Scan::currentCountingComponent() const {
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
        return ERROR("Global scan configuration wrong: per default time based scans -> "
                     "timePerStep needs to be defined/if not time based number of events must be given!");
    return true;
}
