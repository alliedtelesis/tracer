import os
import time
import shutil
import unittest

import tracecollector


class Soak(unittest.TestCase):
    def setUp(self):
        self.t_dir = os.path.join(os.getcwd(), 'test')
        self.t_sock = os.path.join(self.t_dir, 'trace.sock')
        shutil.rmtree(self.t_dir, ignore_errors=True)
        self.collector = tracecollector.TraceCollector(
                            self.t_sock, self.t_dir)
        self.collector.start()

    def test(self):
        env = os.environ
        env['TRACE_UNIX'] = self.t_sock
        #env['TRACE_HOST'] = '127.0.0.1'
        env['TRACE_PORT'] = '8998'
        env['TRACE_INTERCEPT_DIR'] = '/not/a/dir'
        env['BS_PACKAGE_NAME'] = 'test-pkg'
        for i in range(20000):
            pid = os.fork()
            if pid == 0:
                os.execve('trace-exec', ['true', '-c', 'test%s.c' % i], env)

        # Give them time
        time.sleep(5)

        with open(os.path.join(self.t_dir, 'compile_commands.test-pkg.json')) as f:
            count = 1
            for line in f:
                count += 1
        self.assertEqual(count, 20000)

    def tearDown(self):
        self.collector.stop()
        shutil.rmtree(self.t_dir, ignore_errors=True)

if __name__ == '__main__':
    unittest.main()
