#!/usr/bin/env python
# -*- coding: utf-8 -*-

# qtripp
# Copyright (C) 2017 Jan-Piet Mens <jp@mens.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


import beanstalkc           # pip install beanstalkc
import json
import sys

def process_job(body):

    data = json.loads(body)
    print json.dumps(data, indent=4)

    return True

if __name__ == '__main__':
    beanstalk = beanstalkc.Connection(host='localhost', port=11300)
    beanstalk.use('qtripp')
    beanstalk.watch('qtripp')
    beanstalk.ignore('default') 
    print "using:", beanstalk.using()
    print "watching:", beanstalk.watching()

    try:
        while True:
            job = beanstalk.reserve()
            if process_job(job.body) == True:
                job.delete()
    except KeyboardInterrupt:
        sys.exit(1)
    except:
        raise


