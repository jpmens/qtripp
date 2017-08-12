#!/usr/bin/env python

import os
import sys
import glob
import yaml
import datetime
import codecs
import json
from jinja2 import Environment, FileSystemLoader

TEMPLATE_ENV = Environment(
    autoescape=False,
    trim_blocks=True,
    loader=FileSystemLoader(".")
    )

def render_template(filename, context):
    return TEMPLATE_ENV.get_template(filename).render(context)


def loadf(filename):

    try:
        f = codecs.open(filename, 'r', 'utf-8')
        str = f.read()
        doc = yaml.load(str)
        f.close()

        if not doc:
            print "Can't load file %s" % filename

        return doc

    except KeyboardInterrupt:
        sys.exit(1)
    except:
        print >> sys.stderr, "*********** File == ", filename
        raise


if __name__ == '__main__':
    try:
        (otype, path) = sys.argv[1:]
    except:
        print "Usage: report-type filename"
        sys.exit(1)

    doc = loadf(path)
    # print json.dumps(doc, indent=4)

    for o in doc:
        if otype in o:
            data = {
                otype : o[otype],
            }
            # print data
            output = render_template('%s.j2' % otype, data)
            print output
            sys.exit(0)

