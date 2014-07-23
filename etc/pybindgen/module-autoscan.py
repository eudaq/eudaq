#! /usr/bin/env python

import sys

from pybindgen import FileCodeSink
from pybindgen.gccxmlparser import ModuleParser

def my_module_gen():
    module_parser = ModuleParser('libPyIndexReader', '::')
    module_parser.parse([sys.argv[1]], includes=['"eudaq/IndexReader.hh"'], pygen_sink=FileCodeSink(sys.stdout))

if __name__ == '__main__':
    my_module_gen()
