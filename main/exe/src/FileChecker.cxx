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


using namespace eudaq;
using namespace std;

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

const TLUEvent * as_tlu(const DetectorEvent & e, unsigned i_sub)
{
    return dynamic_cast<const TLUEvent *>(e.GetEvent(i_sub));
}

const RawDataEvent * as_raw(const DetectorEvent & e, unsigned i_sub)
{
    return dynamic_cast<const RawDataEvent *>(e.GetEvent(i_sub));
}

int main(int /*argc*/, const char ** argv) {

    EUDAQ_LOG_LEVEL("WARN");

    OptionParser opt("FileChecker.exe", "1.0", "", 1);
    Option<string> opt_input_pattern(opt,
        "i", "input-pattern", "../data/run$6R.raw",
        "STRING", "input filepath pattern");

    try {
        opt.Parse(argv);
    } catch (...) {
        return opt.HandleMainException();
    }

    cout << left << setw(6)  << "run"        << "  "
         << left << setw(5)  << "valid"      << "  "
         << left << setw(10) << "num_events" << "  "
         << left << setw(25) << "contains"   << "  "
         << left << setw(26) << "errors"
         << endl;
    cout << setfill('-')
         << left << setw(6)  << "-" << "  "
         << left << setw(5)  << "-" << "  "
         << left << setw(10) << "-" << "  "
         << left << setw(25) << "-" << "  "
         << left << setw(26) << "-"
         << endl;
    cout << setfill(' ') << boolalpha;

    for (size_t i_run = 0; i_run < opt.NumArgs(); ++i_run) {

        const string & run = opt.GetArg(i_run);
        const string & input_pattern = opt_input_pattern.Value();

        long num_events = -1;
        map<string, unsigned> subevents;

        string errors;

        try {
            // always synchronize based on the trigger id
            FileReader reader(run, input_pattern, true);

            const DetectorEvent & bore = reader.GetDetectorEvent();
            if (!bore.IsBORE()) errors += "first event is not BORE. ";
            // TODO do we need to initialize the PluginManager?
            // no conversion to StandardEvents occurs

            for (num_events = 0; reader.NextEvent(); ++num_events) {
                const DetectorEvent & e = reader.GetDetectorEvent();

                if (e.IsEORE()) break;

                // count the number each subevent occurs
                for (size_t i_sub = 0; i_sub < e.NumEvents(); ++i_sub) {
                    if (const TLUEvent * st = as_tlu(e, i_sub)) {
                        increase(subevents, string("TLU"));
                        continue;
                    } else if (const RawDataEvent * sr = as_raw(e, i_sub)) {
                        increase(subevents, sr->GetSubType());
                    }
                }
            }

            if (num_events <= 0) errors += "no events in the file. ";
        } catch(...) {
            errors += "read error. ";
//            op.HandleMainException();
        }

        // subevents should have the same count as the detector events
        map<string, unsigned>::const_iterator sub;
        string contains;
        bool has_consistent_subevents = true;
        for (sub = subevents.begin(); sub != subevents.end(); ++sub) {
            if (!contains.empty()) contains += ",";
            contains += sub->first;
            has_consistent_subevents &= (sub->second == num_events);
        }
        if (!has_consistent_subevents) errors += "inconsistent subevent counts";

        cout << right << setw(6)  << run            << "  "
             << right << setw(5)  << errors.empty() << "  "
             << right << setw(10) << num_events     << "  "
             << left  << setw(25) << contains       << "  "
             << left  << setw(26) << errors
             << endl;
    }

    return 0;
}

