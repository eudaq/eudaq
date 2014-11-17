#! /usr/bin/env python

import sys,os

from pybindgen import FileCodeSink
from pybindgen.gccxmlparser import ModuleParser

def my_module_gen():
    libName = 'libPy' + sys.argv[2]
    className = sys.argv[2] + '.hh'
    includeFile = sys.argv[1] + '/' + className
    module_parser = ModuleParser( libName, '::')
    module_parser.parse([includeFile], includes=['"' + includeFile + '"'], pygen_sink=FileCodeSink(sys.stdout))

if __name__ == '__main__':
    savedPath = os.getcwd()
    os.chdir( '../../main/include')
    my_module_gen()
    os.chdir( savedPath )

