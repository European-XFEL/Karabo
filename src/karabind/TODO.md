
General
=======

- Lets try to provide better folder structure (Karabind should not be flat?)
  * Elements in their own file together with MACRO? 
- Every subitem is (ideally) a single merge request
  * Hash and Schema are very large


# util
* [x] exportPyUtilHash()
    [ ] numpy integration

- [ ] exportPyUtilSchema()

  * [ ] SchemaWrapper
    * Schema itself and namespace
    * Try to split this into 2-4 MR's

  * [ ] ValidatorWrapper
    
  * [ ] Elements
    * [ ] NodeElementWrapper
    * [ ] ChoiceElementWrapper
    * [ ] ListElementWrapper
    * [ ] InputElementWrapper
    * [ ] OutputElementWrapper
    * [ ] OverwriteElementWrapper
    * [ ] TableElementWrapper
      * [ ] ReadOnlyTableWrapper
    * [ ] NDArrayElementWrapper
    * [ ] HashFilterWrap


- [ ] exportPyUtilClassInfo()
- [ ] exportPyUtilTrainstamp()
- [ ] exportPyUtilDateTimeString()
- [ ] exportPyUtilEpochstamp()
- [ ] exportPyUtilException() // Simple?
- [ ] exportPyUtilTimestamp()
- [ ] exportPyUtilTimeDuration()
- [ ] exportPyUtilDims()
- [ ] exportPyUtilNDArray()
- [?] exportPyUtilDetectorGeometry() 
  * Don't transport initially
- [X] exportPyUtilRollingWindowStatistics()
  * Don't transport to karabind

- [ ] exportPyUtilStateElement()
- [ ] exportPyUtilAlarmConditionElement()

# io
- [ ] exportPyIo()
- [ ] exportPyIoFileTools()

- [ ] exportPyIoOutput()
- [ ] exportPyIoInput()
- [ ] exportPyIoTextSerializer()
- [ ] exportPyIoBinarySerializer()
- [?] exportPyIoH5File()
  * Maybe don't transport this one

# xms
- [ ] exportPyXmsInputOutputChannel() // Large
- [ ] exportPyXmsSignalSlotable()
- [ ] exportPyXmsSlotElement()

# core
- [ ] exportPyCoreDeviceClient() // Careful
- [ ] exportPyCoreLock()

# log
- [ ] exportPyLogLogger()

# net
- [ ] exportp2p()
