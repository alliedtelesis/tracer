#!/usr/bin/env python
import argparse
import json
import fnmatch
import os
import threading
import SocketServer
import sys
import shlex
from Queue import Queue


class TraceHandler(SocketServer.StreamRequestHandler):
    def get_file(self, command):
        argv = shlex.split(command)

        cfiles = fnmatch.filter(argv, '*.c')
        if cfiles:
            return cfiles[0]

        sfiles = fnmatch.filter(argv, '*.s')
        if sfiles:
            return sfiles[0]

    def handle(self):
        data = self.rfile.read()
        try:
            package, cwd, cmd = data.split('\t')
            c_file = self.get_file(cmd)
            if c_file and ('conftest.c' not in c_file):
                self.server.queue.put(
                    (package, {'command': cmd,
                               'directory': cwd,
                               'file' : c_file,})
                )
        except:
            print stderr, 'Received malformed trace "%s"' % data


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Trace Collector')
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=44444)
    parser.add_argument('--unix')
    args = parser.parse_args()

    HOST = args.host
    PORT = args.port


    q = Queue()
    s = SocketServer.TCPServer((HOST, PORT), TraceHandler)
    s.queue = q
    server_thread = threading.Thread(target=s.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    print 'Listening on %s:%i' % (HOST, PORT)

    try:
        while 1:
            package, trace = q.get()
            print trace
            q.task_done()
    except KeyboardInterrupt:
        pass
    finally:
        s.shutdown()

