# qtripp

The _Queclink Track (air) Interface Protocol Processor_ is a TCP GPRS server for Queclink devices: _qtripp_ obtains GPS positions from these devices and publishes it over MQTT in [OwnTracks JSON format](http://owntracks.org/booklet/tech/json/) as `_location_ objects.

## requirements

* an Internet-facing server with an open TCP port
* a Queclink device. We have tested GV65, GV55, GV200MT, and GV500. Others ought to be no problem, though you might have to adapt device configuration from the Protocol documentation.
* an MQTT broker (see the [OwnTracks Booklet](http://owntracks.org/booklet/guide/broker/))
* a bit of patience

## features

* copious debugging
* ignore specific reports
* configurable reports per/device and per/firmware basis
* fast
* OwnTracks JSON support
* MQTT
* list devices connected (console & MQTT)
* statistics over MQTT
* statistics dump including _subtype_ stats and _IMEI_ stats.

```
-t owntracks/qtripp/*/cmd -m list
-t owntracks/qtripp/*/cmd -m stats
-t owntracks/qtripp/*/cmd -m dump
```

## credits

* [uthash](https://troydhanson.github.io/uthash/), by Troy D. Hanson
* [json.c](https://ccodearchive.net/info/json.html), by Joey Adams
* [ini.c](https://github.com/benhoyt/inih), by Ben Hoyt
* [mongoose](https://github.com/cesanta/mongoose), by Cesanta
