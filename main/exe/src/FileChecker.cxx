/**
 * check if a raw file can be opened, synced and list the event types
 *
 * @author      Niklaus Berger <nberger@physi.uni-heidelberg.de>
 * @author      Moritz Kiehn <kiehn@physi.uni-heidelberg.de>
 * @date        2013
 *
 */

#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "eudaq/DetectorEvent.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/TLUEvent.hh"
#include "eudaq/MultiFileReader.hh"



/** increase the counter by one. default to 1 for the first access
 * adapted from http://stackoverflow.com/questions/2333728/stdmap-default-value
 */
template <typename K, typename V>
void increase(std::map<K,V> & m, const K & key)
{
    typename std::map<K,V>::const_iterator it = m.find(key);
    if (it == m.end()) {
        m[key] = 1;
    } else {
        m[key] += 1;
    }
}

const eudaq::TLUEvent * as_tlu(const eudaq::DetectorEvent & e, unsigned i_sub)
{
    return dynamic_cast<const eudaq::TLUEvent *>(e.GetEvent(i_sub));
}

const eudaq::RawDataEvent * as_raw(const eudaq::DetectorEvent & e, unsigned i_sub)
{
    return dynamic_cast<const eudaq::RawDataEvent *>(e.GetEvent(i_sub));
}

int main(int /*argc*/, const char ** argv) {

    EUDAQ_LOG_LEVEL("WARN");

    eudaq::OptionParser opt("FileChecker.exe", "1.0", "", 1);
    eudaq::Option<std::string> opt_input_pattern(opt,
        "i", "input-pattern", "../data/run$6R.raw",
        "STRING", "input filepath pattern");
    eudaq::OptionFlag async(opt, "a", "nosync", "Disables Synchronisation with TLU events");

    try {
        opt.Parse(argv);
    } catch (...) {
        return opt.HandleMainException();
    }

    std::cout << std::left << std::setw(6)  << "run"        << "  "
         << std::left << std::setw(5)  << "valid"      << "  "
         << std::left << std::setw(10) << "num_events" << "  "
         << std::left << std::setw(25) << "contains"   << "  "
         << std::left << std::setw(26) << "errors"
         << std::endl;
    std::cout << std::setfill('-')
         << std::left << std::setw(6)  << "-" << "  "
         << std::left << std::setw(5)  << "-" << "  "
         << std::left << std::setw(10) << "-" << "  "
         << std::left << std::setw(25) << "-" << "  "
         << std::left << std::setw(26) << "-"
         << std::endl;
    std::cout << std::setfill(' ') << std::boolalpha;

    for (size_t i_run = 0; i_run < opt.NumArgs(); ++i_run) {

        const std::string & run = opt.GetArg(i_run);
        const std::string & input_pattern = opt_input_pattern.Value();

        int32_t num_events = -1;
        std::map<std::string, unsigned> subevents;

        std::string errors;

        try {
            // always synchronize based on the trigger id
	    eudaq::multiFileReader reader(!async.Value());
            reader.addFileReader(run, input_pattern);

            const eudaq::DetectorEvent & bore = reader.GetDetectorEvent();
            if (!bore.IsBORE()) errors += "first event is not BORE. ";
            // TODO do we need to initialize the PluginManager?
            // no conversion to StandardEvents occurs

            for (num_events = 0; reader.NextEvent(); ++num_events) {
                const eudaq::DetectorEvent & e = reader.GetDetectorEvent();

                if (e.IsEORE()) break;

                // count the number each subevent occurs
                for (size_t i_sub = 0; i_sub < e.NumEvents(); ++i_sub) {
                    if (const eudaq::TLUEvent * st = as_tlu(e, i_sub)) {
                        increase(subevents, std::string("TLU"));
                        continue;
                    } else if (const eudaq::RawDataEvent * sr = as_raw(e, i_sub)) {
                        increase(subevents, sr->GetSubType());
                    }
                }
            }

            if (num_events <= 0) errors += "no events in the file. ";
        } catch(const eudaq::Exception & e) {
	  errors += e.what();
        }

        // subevents should have the same count as the detector events
        std::string contains;
        bool has_consistent_subevents = true;
        for (const auto& sub : subevents) {
            if (!contains.empty()) contains += ",";
            contains += sub.first;
            has_consistent_subevents &= (sub.second == num_events);
        }
        if (!has_consistent_subevents) errors += "inconsistent subevent counts";

        std::cout << std::right << std::setw(6)  << run            << "  "
             << std::right << std::setw(5)  << errors.empty() << "  "
             << std::right << std::setw(10) << num_events     << "  "
             << std::left  << std::setw(25) << contains       << "  "
             << std::left  << std::setw(26) << errors
             << std::endl;
    }

    return 0;
}

