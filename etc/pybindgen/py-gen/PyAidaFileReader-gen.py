from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers


import pybindgen.settings
import warnings

class ErrorHandler(pybindgen.settings.ErrorHandler):
    def handle_error(self, wrapper, exception, traceback_):
        warnings.warn("exception %r in wrapper %s" % (exception, wrapper))
        return True
pybindgen.settings.error_handler = ErrorHandler()


import sys

def module_init():
    root_module = Module('libPyAidaFileReader', cpp_namespace='::')
    root_module.add_include('"eudaq/AidaFileReader.hh"')
    return root_module

def register_types(module):
    root_module = module.get_root()
    
    
    ## Register a nested module for the namespace eudaq
    
    nested_module = module.add_cpp_namespace('eudaq')
    register_types_eudaq(nested_module)
    
    
    ## Register a nested module for the namespace std
    
    nested_module = module.add_cpp_namespace('std')
    register_types_std(nested_module)
    

def register_types_eudaq(module):
    root_module = module.get_root()
    
    module.add_class('AidaFileReader')

def register_types_std(module):
    root_module = module.get_root()
    

def register_methods(root_module):
    register_EudaqAidaFileReader_methods(root_module, root_module['eudaq::AidaFileReader'])
    return

def register_EudaqAidaFileReader_methods(root_module, cls):
    cls.add_constructor([param('eudaq::AidaFileReader const &', 'arg0')])
    cls.add_constructor([param('std::string const &', 'filename')])
    cls.add_method('Filename', 
                   'std::string', 
                   [], 
                   is_const=True)
    cls.add_method('RunNumber', 
                   'unsigned int', 
                   [], 
                   is_const=True)
    cls.add_method('getJsonConfig', 
                   'std::string', 
                   [])
    cls.add_method('getJsonPacketInfo', 
                   'std::string', 
                   [])
    cls.add_method('readNext', 
                   'bool', 
                   [])
    return

def register_functions(root_module):
    module = root_module
    register_functions_eudaq(module.get_submodule('eudaq'), root_module)
    register_functions_std(module.get_submodule('std'), root_module)
    return

def register_functions_eudaq(module, root_module):
    return

def register_functions_std(module, root_module):
    return

def main():
    out = FileCodeSink(sys.stdout)
    root_module = module_init()
    register_types(root_module)
    register_methods(root_module)
    register_functions(root_module)
    root_module.generate(out)

if __name__ == '__main__':
    main()

