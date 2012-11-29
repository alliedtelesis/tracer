#!/usr/bin/env python
import argparse
import json
import fnmatch
import os
import time
import threading
import SocketServer
import sys
import shlex
import Queue


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
            if c_file and ('conftest.' not in c_file):
                self.server.queue.put(
                    (package, {'command': cmd,
                               'directory': cwd,
                               'file' : c_file,})
                )
        except:
            print stderr, 'Received malformed trace "%s"' % data

class CompileCommandStorage(object):
    def __init__(self, path):
        if not os.access(path, os.W_OK):
            try:
                os.mkdir(path)
            except IOError:
                raise IOError('Output directory is not writable %s' % path)
        self.path = path

        self.handles = {}

    def _create_package_file(self, package):
        if not self.handles.has_key(package):
            f = open(os.path.join(self.path, 'compile_commands.%s.json' % package), 'w')
            f.write('[')
            self.handles[package] = f

    def _close_package_files(self):
        for f in self.handles.itervalues():
            f.write(']')
            f.close()

    def close(self):
        self._close_package_files()

    def store(self, package, command):
        if not self.handles.has_key(package):
            self._create_package_file(package)
        f = self.handles[package]
        if f.tell() != 1:
            f.write(',')
        f.write(json.dumps(command))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Trace Collector')
 
    parser.add_argument(
        'port', type=int, help='Port to bind to'
    )
    parser.add_argument(
        '--addr', default='localhost', help='Address to bind to'
    )
    parser.add_argument(
        '-o', nargs=1, required=True, metavar='PATH', help='Output directory'
    )
    args = parser.parse_args()

    HOST = args.addr
    PORT = args.port
    O_PATH = args.o[0]

    q = Queue.Queue()
    s = SocketServer.TCPServer((HOST, PORT), TraceHandler)
    s.queue = q
    server_thread = threading.Thread(target=s.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    print 'Listening on %s:%i' % (HOST, PORT)

    store = CompileCommandStorage(O_PATH)

    try:
        while 1:
            try:
                store.store(*q.get(block=False))
                q.task_done()
            except Queue.Empty:
                time.sleep(0.5)
    except KeyboardInterrupt:
        pass
    finally:
        store.close()
        s.shutdown()

