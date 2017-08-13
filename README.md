# qtripp

Queclink Track (air) Interface Protocol Processor and OwnTracks

## features

* copious debugging
* ignore specific reports
* configurable reports per/device and per/firmware basis
* fast
* OwnTracks JSON support
* MQTT
* list devices connected (console & MQTT)
* periodic statistics over MQTT

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
