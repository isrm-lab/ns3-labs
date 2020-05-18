## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    obj = bld.create_ns3_program('lab3', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-03-04-capacity/lab3.cc'

    obj = bld.create_ns3_program('lab3m', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-03-04-capacity/lab3m.cc'

    obj = bld.create_ns3_program('lab4-legacy', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab4.cc'

    obj = bld.create_ns3_program('lab5', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-05-06-mcs/lab5.cc'

    obj = bld.create_ns3_program('lab6-7-cw', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-07-08-dcf/lab6-7-cw.cc'

    obj = bld.create_ns3_program('lab9', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-09-jain/lab9.cc'

    obj = bld.create_ns3_program('model-colocviu', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'model-colocviu/model-colocviu.cc'

    obj = bld.create_ns3_program('colocviu', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'colocviu/colocviu.cc'

    obj = bld.create_ns3_program('lab10', ['wifi', 'applications', 'internet', 'flow-monitor'])
    obj.source = 'lab-10-multirate/lab10.cc'
