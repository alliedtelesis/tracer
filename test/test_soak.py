import os
import glob
import json
import time
import random
import signal
import shutil
import subprocess
import tempfile
import unittest


ROOT = os.path.dirname(os.path.dirname(__file__))
CTRACE = os.path.join(ROOT, 'ctrace', 'ctrace')
CTRACED = os.path.join(ROOT, 'ctraced', 'ctraced.py')


class Soak(unittest.TestCase):

    def random_command(self, i):
        ext = ['.c', '.c', '.c', '.cpp', '.cxx']
        somedefs = [
            '-D_BSD_SOURCE', '-D_XOPEN_SOURCE=500', '-D_SVID_SOURCE',
            '-D_POSIX_SOURCE', '-D_GNU_SOURCE', '-D_POSIX_C_SOURCE=199506L',
            '-D_XOPEN_SOURCE_EXTENDED', '-D_REENTRANT', '-D_THREAD_SAFE',
            '-D_FORTIFY_SOURCE'
        ]
        flags = ['-Wall', '-fno-inline', '-fno-builtin', '-funroll-loops']

        cmd = [
            'true',
            random.choice(somedefs),
            random.choice(flags),
            '-c', 'file-%i%s' % (i, random.choice(ext))
        ]
        return cmd

    def random_package(self):
        packages = [
            'sudo', 'nano', 'emacs', 'vim', 'init', 'grub', 'libc/eglibc'
        ]
        return random.choice(packages)

    def soak(self, number):
        test_tmp = tempfile.mkdtemp(prefix='ctraced-test')
        try:
            un_path = os.path.join(test_tmp, 'trace.sock')
            db_path = os.path.join(test_tmp, 'trace.sqlite')
            cc_path = os.path.join(test_tmp, 'traces')
            os.makedirs(cc_path)
            try:
                ctraced = subprocess.Popen(
                    [CTRACED, un_path, db_path, cc_path],
                )

                # Wait for ctraced to start
                time.sleep(3)

                env = os.environ
                env['CTRACE_SOCKET'] = un_path

                s_time = time.time()
                for i in range(number):
                    env['PACKAGE'] = self.random_package()
                    cmd = [CTRACE]
                    cmd.extend(self.random_command(i))
                    subprocess.Popen(args=cmd, env=env).wait()
                f_time = time.time()

                # Wait for connection queue to empty
                time.sleep(1)
            finally:
                ctraced.send_signal(signal.SIGINT)

            # Wait for json dump
            ctraced.wait()

            count = 0
            for fn in glob.glob(os.path.join(cc_path, '*')):
                with open(fn) as f:
                    count += len(json.load(f))

            loss = number / float(count)
            rate = float(number) / (f_time - s_time)

            if count != number:
                print('Loss of %.06f%% with rate of %.2f/s' % (loss, rate))
            self.assertEqual(count, number)
        finally:
            shutil.rmtree(test_tmp, ignore_errors=True)

    def test_10(self):
        self.soak(10)

    def test_100(self):
        self.soak(100)

    def test_1000(self):
        self.soak(1000)

    def test_10000(self):
        self.soak(10000)
