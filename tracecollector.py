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

        cfiles = reduce(
            lambda x, y: x+y,
            [fnmatch.filter(argv, '*.c'),
             fnmatch.filter(argv, '*.cpp'),
             fnmatch.filter(argv, '*.cxx'),
             fnmatch.filter(argv, '*.c++'),
             fnmatch.filter(argv, '*.s')]
        )
        if cfiles:
            return cfiles[0]

    def handle(self):
        data = self.rfile.read()
        self.rfile.close()
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
            sys.strerr.write('Received malformed trace "%s"' % data)

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
            f.write('[]')
            self.handles[package] = f

    def _close_package_files(self):
        for f in self.handles.itervalues():
            f.close()

    def close(self):
        self._close_package_files()

    def store(self, package, command):
        if not self.handles.has_key(package):
            self._create_package_file(package)
        f = self.handles[package]
        # overwrite trailing close bracket
        f.seek(-1, 1)
        if f.tell() != 1:
            f.write(',\n ')
        f.write(json.dumps(command))
        f.write(']')


class TraceCollector(object):
    def __init__(self, host, output_dir, port=False):
        self.host = host
        self.port = port
        self.o_dir = output_dir

        self.queue = Queue.Queue()
        self.store = CompileCommandStorage(self.o_dir)

    def _start_in_thread(self):
        if not self.port:
            self.server = SocketServer.UnixStreamServer(self.host, TraceHandler)
            sock = self.host
        else:
            self.server = SocketServer.TCPServer((self.host, self.port), TraceHandler)
            self.server.allow_reuse_address = True
            sock = '%s:%i' % (self.host, self.port)

        self.server.request_queue_size = 1024
        self.server.queue = self.queue
        self.in_thread = threading.Thread(target=self.server.serve_forever)
        self.in_thread.daemon = True
        self.in_thread.start()
        print 'TraceCollector listening on %s' % sock

    def _start_out_thread(self):
        self.out_thread = threading.Thread(target=self._out_thread)
        self.out_thread.start()

    def _out_thread(self):
        try:
            while self.run == True:
                try:
                    self.store.store(*self.queue.get(block=False))
                    self.queue.task_done()
                except Queue.Empty:
                    time.sleep(0.1)
        finally:
            self.store.close()
            self.server.shutdown()

    def start(self):
        self.run = True
        self._start_out_thread()
        self._start_in_thread()

    def stop(self):
        self.run = False
        self.out_thread.join()
        if not self.port:
            # Remove the Unix socket
            os.remove(self.host)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Trace Collector')

    parser.add_argument(
        'addr', help='Address to bind to'
    )
    parser.add_argument(
        '--tcp', default=False, type=int, metavar='PORT', help='TCP Port to bind to'
    )
    parser.add_argument(
        '-o', nargs=1, required=True, metavar='PATH', help='Output directory'
    )
    args = parser.parse_args()

    tc = TraceCollector(args.addr, args.o[0], port=args.tcp)
    tc.start()

