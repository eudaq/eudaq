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

  plane_sync(int nevts_per_bin): _nevts_per_bin(nevts_per_bin) {}

  int get_resync_value(int plane, int evt) {
    if(_desync_data.find(plane) == _desync_data.end()) {
        return 0;
    } else {
        return _desync_data[plane].at(evt/_nevts_per_bin);
    }
  }

  int is_good_evt(int evt) {
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
      if(v[ix] != current_shift) {
        _good_blocks[ix-1] = false;
        _good_blocks[ix] = false;
        current_shift = v[ix];
      }
    }
  }

  void write_json(std::string const & outpath) {
    jsoncons::json j;
    jsoncons::json jplane;
    j["nevts_per_bin"] =_nevts_per_bin;
    for(auto const & [id, v]: _desync_data) {
        jplane[std::to_string(id)] = jsoncons::json{v};
    }
    j["plane_shifts"] = jplane;
    j["good_blocks"] = jsoncons::json{_good_blocks};
    std::cout << jsoncons::pretty_print(j) << std::endl;
  }

  std::vector<bool> _good_blocks;
  std::map<int, std::vector<int>> _desync_data;
  int _nevts_per_bin = 0;
  bool _has_data = false;
  std::size_t _block_size = 0;
};

#endif // EUDAQ_INCLUDED_Desynccorr_SyncObject