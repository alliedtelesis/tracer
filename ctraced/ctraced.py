#!/usr/bin/env python3
import json
import os
import sqlite3
import socketserver
import sys

from pool import PooledProcessMixIn


class TraceStore(object):
    def __init__(self, path):
        self.path = path

        new = not os.access(path, os.F_OK)
        self.db = sqlite3.connect(path)
        if new:
            self.db.execute("""
                CREATE TABLE commands (
                    command text,
                    directory text,
                    file text,
                    package text,
                    UNIQUE(directory, file, package) ON CONFLICT REPLACE
                );
            """)

    def store(self, ccmd):
        self.db.execute(
            'INSERT INTO commands VALUES (?, ?, ?, ?);',
            [ccmd['command'], ccmd['directory'], ccmd['file'], ccmd['package']]
        )

    def dump(self, path):
        if not os.access(path, os.W_OK):
            os.makedirs(path)
        packages = self.db.execute('SELECT DISTINCT package FROM commands;')
        packages = [p[0] for p in packages]
        for package in packages:
            commands = self.db.execute(
                'SELECT command, directory, file '
                'FROM commands WHERE package=?;',
                [package]
            )
            def ccmd(c, d, f): return {'command': c, 'directory': d, 'file': f}

            package_safe = package.replace('/', '-')
            p = os.path.join(path, 'compile_commands.%s.json' % package_safe)
            with open(p, 'w') as f:
                json.dump([ccmd(*c) for c in commands], f)

    def close(self):
        self.db.commit()
        self.db.close()


class TraceHandler(socketserver.StreamRequestHandler):
    def handle(self):
        ccmd = json.load(self.rfile)
        self.server.trace_store.store(ccmd)


class TraceServer(socketserver.UnixStreamServer, PooledProcessMixIn):
    pass


class TraceCollector(object):
    def __init__(self, un_path, db_path, cc_path):
        self.un_path = un_path
        self.db_path = db_path
        self.cc_path = cc_path

        self.server = TraceServer(self.un_path, TraceHandler)
        self.server.request_queue_size = 1024

    def run(self):
        self.server.trace_store = TraceStore(self.db_path)
        try:
            self.server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            self.server.trace_store.dump(self.cc_path)
            self.server.trace_store.close()
            os.unlink(self.un_path)


if __name__ == '__main__':
    tc = TraceCollector(sys.argv[1], sys.argv[2], sys.argv[3])
    tc.run()
