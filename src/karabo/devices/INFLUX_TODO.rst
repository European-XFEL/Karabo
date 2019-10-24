TODO
====

Here below id the TODO list for the influxDbLogger Project

[ ] Add an influxDb service to the CI on gitlab
[ ] Write the documentation on:
  [ ]  How we organise the DB (one db per topic, one installation in production, no central installation in office network).
  [ ]  Measurements (one per deviceId)
  [ ]  How to save a device schema
  [ ]  Protocol to be used (TCP/http no https)
  [ ]  How to getConfigurationFromPast
  [ ]  how to get property history
  [ ]  how to save the user field
  [ ]  how to log events like instanceNew and instanceGone (one measurement table per events?)
  [ ]  What to do with vectors
  [ ]  What to do with tables
  [ ]  check the possibility to save the schema of a device in a separate DB/table and index it by a checksum
  [ ]  Evaluate the possibility of using influx db as a backend for the configuration DB
  [ ]  Evaluate possible improvements to the current poll for logs.

[ ]  Create a base Class for DataLoggers

[ ]  InfluxDBLogger implement all features documented
  [ ]  can create a table
  [ ]  can save new configurations
  [ ]  can save schemas
[ ]  InfluxDBLogReader implement all features documented
  [ ]  can retrieve schemas
  [ ]  can retrieve configurations from past
  [ ]  can retrieve property history
[ ]  add a karabo-startDataArchiveDB script for local installation
[ ]  add a karabo-stopDataArchiveDB script for local installations
[ ]  Current Integration test passes on the new class