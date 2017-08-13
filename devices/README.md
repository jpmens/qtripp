## Devices

Queclink's @Track protocol provides lots of different "records", each identified by a five-letter code, e.g. `GTFRI`, and the records themselves contain CSV values. Within these records, the data we look for is contained at different positions in the CSV, e.g. sometimes `lat` is at slot 8, other times at slot 10.

The YAML file describes the fields within the individual records and their positions. This is generated into include files (`*.i`) containing a struct which is later hashed at runtime.

Similarly, the YAML also has a list of lookup tables, namely `includes`, `reports`, and `models` which are also hashed for fast use.
