#ifndef Event_declarationMacros_h__
#define Event_declarationMacros_h__




#define EUDAQ_DECLARE_EVENT(type)              \
  public:                                      \
static  MainType_t eudaq_static_id();          \
virtual  MainType_t get_id() const {           \
  return eudaq_static_id();                    \
}                                              \
private:                                       \
static const int EUDAQ_DUMMY_VAR_DONT_USE = 0

#define EUDAQ_DEFINE_EVENT(type, id)                \
   type::MainType_t type::eudaq_static_id() {       \
    static const  type::MainType_t id_(id);         \
    return id_;                                     \
        }                                           \
namespace _eudaq_dummy_ {                           \
  static eudaq::RegisterEventType<type> eudaq_reg;	\
}                                                   \
static const int EUDAQ_DUMMY_VAR_DONT_USE = 0

#endif // Event_declarationMacros_h__