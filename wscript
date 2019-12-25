## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    obj = bld.create_ns3_program('lab3', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab3.cc'

    obj = bld.create_ns3_program('lab4', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab4.cc'

    obj = bld.create_ns3_program('lab6-7-cw', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-06/lab6-7-cw.cc'

    obj = bld.create_ns3_program('lab8', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-08/lab8.cc'

    obj = bld.create_ns3_program('lab10', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab10.cc'