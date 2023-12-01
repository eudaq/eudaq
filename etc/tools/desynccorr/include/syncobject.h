#ifndef EUDAQ_INCLUDED_Desynccorr_SyncObject
#define EUDAQ_INCLUDED_Desynccorr_SyncObject

#include "jsoncons/json.hpp"

template <typename T>
class overflowbuffer {
  public:
    overflowbuffer(std::size_t N, T t = T()): _depth(N) {
      _c.resize(2*_depth+1, t);
    }
    void push(T& v) {
      _c.pop_front();
      _c.emplace_back(v);
    } 
    T & get(int index) {
      return _c.at(_depth+index);
    }

  private:
    std::deque<T> _c;
    std::size_t _depth;
};

struct plane_sync {

  plane_sync(int run_number, int nevts_per_bin, int max_shift): _run_number(run_number), _nevts_per_bin(nevts_per_bin), _max_shift(max_shift) {}

  plane_sync(std::string fname) {
    std::ifstream is(fname);  
    jsoncons::json j = jsoncons::json::parse(is);
    _nevts_per_bin = j["nevts_per_bin"].as<int>();
    _run_number = j["run_number"].as<int>();
    _max_shift = j["max_shift"].as<int>();
    _good_blocks = j["good_blocks"].as<std::vector<bool>>();
    _block_size = _good_blocks.size();
    jsoncons::json jp =  j["plane_shifts"];
    for(const auto& p : jp.object_range()) {
        auto key = std::stoi(std::string{p.key()});
        auto val = p.value().as<std::vector<int>>();
        _desync_data[key] = val;
        if(!_has_data) {
            _has_data = true;
        }
        if( (_block_size != val.size()) || val.size() < 3) {
            throw std::runtime_error("Adding plane "+std::to_string(key)+" failed because of wrong size. Passed size is : "
            +std::to_string(val.size())+" and expected size is larger than 3 and/or "+std::to_string(_block_size));
        }      
    }
  }

  int get_resync_value(int plane, int evt) const {
    if(_desync_data.find(plane) == _desync_data.end()) {
        return 0;
    } else {
        return _desync_data.at(plane).at(evt/_nevts_per_bin);
    }
  }

  int is_good_evt(int evt) const {
    return _good_blocks.at(evt/_nevts_per_bin);
  }

  void add_plane(int plane, std::vector<int> const & v) {
     if( (_has_data && _block_size != v.size()) || v.size() < 3) {
        throw std::runtime_error("Adding plane "+std::to_string(plane)+" failed because of wrong size. Passed size is : "
        +std::to_string(v.size())+" and expected size is larger than 3 and/or "+std::to_string(_block_size));
        return;
    } else if(!_has_data) {
        _has_data = true;
        _block_size = v.size();
        _good_blocks.resize(v.size(), true);
        _good_blocks[0] = false;
    }
    _desync_data[plane] = v;
    auto current_shift = v[1];
    for(std::size_t ix = 2; ix < v.size(); ix++) {
      if( (v[ix] != current_shift) || (v[ix] == -_max_shift) ) {
        _good_blocks[ix-1] = false;
        _good_blocks[ix] = false;
        current_shift = v[ix];
      }
    }
  }

  void write_json(std::string const & outpath) const {
    jsoncons::json j;
    jsoncons::json jplane;
    j["run_number"] = _run_number;
    j["max_shift"] = _max_shift;
    j["nevts_per_bin"] = _nevts_per_bin;
    for(auto const & [id, v]: _desync_data) {
        jplane[std::to_string(id)] = jsoncons::json{v};
    }
    j["plane_shifts"] = jplane;
    j["good_blocks"] = jsoncons::json{_good_blocks};
    auto ofile = std::ofstream(outpath, std::ios::out | std::ios::trunc);
    ofile << jsoncons::pretty_print(j);
  }

  int good_events() const {
    return _nevts_per_bin*(_block_size-1);
  }

  int run_number() const {
    return _run_number;
  }

    int max_shift() const {
    return _max_shift;
  }

  private:
  std::vector<bool> _good_blocks;
  std::map<int, std::vector<int>> _desync_data;
  int _nevts_per_bin = 0;
  int _run_number = 0;
  int _max_shift = 0;
  bool _has_data = false;
  std::size_t _block_size = 0;
};

#endif // EUDAQ_INCLUDED_Desynccorr_SyncObject