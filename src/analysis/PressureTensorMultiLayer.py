"""
************************************
**PressureTensorMultiLayer** - Analysis
************************************

This class computes the pressure tensor of the system in `n` layers.
Layers are perpendicular to Z direction and are equidistant(distance is Lz/n).
It can be used as standalone class in python as well as
in combination with the integrator extension ExtAnalyze.

Standalone Usage:
-----------------

>>> pt = espresso.analysis.PressureTensorMultiLayer(system, n, dh)
>>> for i in range(n):
>>>     print "pressure tensor in layer %d: %s" % ( i, pt.compute())

or 

>>> pt = espresso.analysis.PressureTensorMultiLayer(system, n, dh)
>>> for k in range(100):
>>>     integrator.run(100)
>>>     pt.performMeasurement()
>>> for i in range(n):
>>>     print "average pressure tensor in layer %d: %s" % ( i, pt.compute())

Usage in integrator with ExtAnalyze:
------------------------------------

>>> pt           = espresso.analysis.PressureTensorMultiLayer(system, n, dh)
>>> extension_pt = espresso.integrator.ExtAnalyze(pt , interval=100)
>>> integrator.addExtension(extension_pt)
>>> integrator.run(10000)
>>> pt_ave = pt.getAverageValue()
>>> for i in range(n):
>>>   print "average Pressure Tensor = ", pt_ave[i][:6]
>>>   print "          std deviation = ", pt_ave[i][6:]
>>> print "number of measurements  = ", pt.getNumberOfMeasurements()

The following methods are supported:

* performMeasurement()
    computes the pressure tensor and updates average and standard deviation
* reset()
    resets average and standard deviation to 0
* compute()
    computes the instant pressure tensor in `n` layers, return value: [xx, yy, zz, xy, xz, yz]
* getAverageValue()
    returns the average pressure tensor and the standard deviation,
    return value: [xx, yy, zz, xy, xz, yz, +-xx, +-yy, +-zz, +-xy, +-xz, +-yz]
* getNumberOfMeasurements()
    counts the number of measurements that have been computed (standalone or in integrator)
    does _not_ include measurements that have been done using "compute()"
"""

from espresso.esutil import cxxinit
from espresso import pmi

from espresso.analysis.AnalysisBase import *
from _espresso import analysis_PressureTensorMultiLayer

class PressureTensorMultiLayerLocal(AnalysisBaseLocal, analysis_PressureTensorMultiLayer):
    'The (local) compute of pressure tensor.'
    def __init__(self, system, n, dh):
        if not pmi._PMIComm or pmi._MPIcomm.rank in pmi._PMIComm.getMPIcpugroup():
            cxxinit(self, analysis_PressureTensorMultiLayer, system, n, dh)

if pmi.isController:
    class PressureTensorMultiLayer(AnalysisBase):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls =  'espresso.analysis.PressureTensorMultiLayerLocal',
            pmiproperty = [ 'n', 'dh' ]
        )