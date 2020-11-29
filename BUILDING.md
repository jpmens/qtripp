
## Mac OS/X

```console
(
cd devices
/usr/bin/python3 -mvenv venv
source venv/bin/activate
pip install -r requirements.txt
)
make
```

## CentOS

## Debian

apt-get install libcdb-dev tinycdb

## OpenBSD

```console
pkg_add gmake git 
pkg_add python-2.7.14 py-yaml py-jinja2 mosquitto

echo  PYTHON=python2.7 > devices/.make
```


