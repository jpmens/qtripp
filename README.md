# qtripp

The _Queclink Track (air) Interface Protocol Processor_ is a TCP GPRS server for Queclink devices: _qtripp_ obtains GPS positions from these devices and publishes those over MQTT in [OwnTracks JSON format](http://owntracks.org/booklet/tech/json/) as `_location_ objects.

![qtripp](assets/qtripp.png)

## requirements

* an Internet-facing server with an open TCP port
* a Queclink device. We have tested GV65, GV65+, GV55, GV200MT, and GV500. Others ought to be no problem, though you might have to adapt device configurations from the Protocol documentation.
* an MQTT broker (see the [OwnTracks Booklet](http://owntracks.org/booklet/guide/broker/))
* a bit of patience

## features

* configurable MQTT publish topics (device lookup)
* extensible
* copious debugging
* support for device reports with embedded segments (e.g. GTFRI/GTERI)
* ignore specific device reports
* configurable reports per/device and on a per/firmware basis
* fast
* extra JSON data can be merged in to data from devices
* OwnTracks JSON support
* MQTT, TLS, TLS client certificates, user/password authentication
* list devices connected (console & MQTT)
* statistics over MQTT
* statistics dump including _subtype_ stats and _IMEI_ stats.
* (pseudo-) LWT for devices (when a device disconnects, _qtripp_ publishes LWT)
* support for 1-Wire temperature sensors (on GV65/GV65+)
* raw data is copied to file for backup, replay, debugging, etc.
* optional beanstalkd support (requires [beanstalk-client](https://github.com/deepfryed/beanstalk-client)) for mirroring. (sample workers are provided.) beanstalk host/port/tube are configurable; payload is OwnTracks JSON enriched with a field `imei` and a `raw_line` field which contains original ASCII device data (`+RESP:GT ... $`)

## commands

```
-t owntracks/qtripp/*/cmd -m list
-t owntracks/qtripp/*/cmd -m stats
-t owntracks/qtripp/*/cmd -m dump
```

## logging



```
2017-08-16T17:25:55Z 1502904355 pid=564 +++ I=863123456789090 (JP car) M=GV65 np=200 P=310603 C=2 T=RESP:GTERI (scheduled report) LINE=+RESP:GTERI,310603,8...
---------+---------- ----+----- ---+---     +---------------- -+------ +----- +----- +------- +-- +----------  +----------------  +-------------------------
         |               |         |        |                  |       |      |      |        |   |            |                  |
         |               |         |        |                  |       |      |      |        |   |            |                  +-----------------  raw data received
         |               |         |        |                  |       |      |      |        |   |            +------------------------------------  (description of T=)
         |               |         |        |                  |       |      |      |        |   +-------------------------------------------------  T=type/subtype
         |               |         |        |                  |       |      |      |        +-----------------------------------------------------  C=counter (total reports handled so far)
         |               |         |        |                  |       |      |      +--------------------------------------------------------------  P=protov (proto, maj/min)
         |               |         |        |                  |       |      +---------------------------------------------------------------------  np=number of parts (csv)
         |               |         |        |                  |       +----------------------------------------------------------------------------  M=model (from protov)
         |               |         |        |                  +------------------------------------------------------------------------------------  (name)
         |               |         |        +-------------------------------------------------------------------------------------------------------  I=<imei>
         |               |         +----------------------------------------------------------------------------------------------------------------  process ID
         |               +--------------------------------------------------------------------------------------------------------------------------  epoch
         +------------------------------------------------------------------------------------------------------------------------------------------  date/timestamp ISO (Zulu)
```

## beanstalk

1. clone [beanstalk-client](https://github.com/deepfryed/beanstalk-client/) into _qtripp_'s directory, `cd` into that and `make`. (Apply [this fix](https://github.com/deepfryed/beanstalk-client/issues/32) on macOS.)
2. in _qtripp_'s `Makefile`, define `BEANSTALK`

## credits

* [uthash](https://troydhanson.github.io/uthash/), by Troy D. Hanson
* [json.c](https://ccodearchive.net/info/json.html), by Joey Adams
* [ini.c](https://github.com/benhoyt/inih), by Ben Hoyt
* [mongoose](https://github.com/cesanta/mongoose), by Cesanta
* [statsd-c-client](https://github.com/romanbsd/statsd-c-client)

## authors

* Jan-Piet Mens (@jpmens)
* Christoph Krey (@ckrey)
