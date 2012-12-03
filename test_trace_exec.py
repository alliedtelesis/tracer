import cStringIO
import ctypes
import ctypes.util
import unittest
import os
import random
import socket
import tempfile
import threading
import time

ARG_CASES = (
    ("executable",),
    ("executable", "arg1", "arg2"),
    ("executable", "longerarg$umentwithspe & cialchars"),
    ("gcc", "-Wall", "-c", "file.c", "-o", "file.o"),
    # Line breaks? nuh
    ("powerpc-603e-linux-uclibc-gcc", "-c", "-isystem/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/include",  "-I../..", "-I../../pal/api", "-I../../pal/linux", "-I../../lib", "-I../../cal", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion/platform/linux", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion-build/platform/linux", "--sysroot=/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/new/install/include/", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/hslhw", "-Wformat=2", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/glib-2.0", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/lib/glib-2.0/include", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/openhpi", "-Os", "-g2", "-mcpu=603e", "-DATL_CHANGE", "-Wall", "-funsigned-char", "-fno-signed-char", "-fno-strict-aliasing", "-Werror", "-Wno-pointer-sign", "-Wlong-long", "-Wnested-externs", "-I../../lib/fm", "-I../../lib/memmgr", "-I../../hal/PBR", "-I../../imi/util", "-I../../hal", "-I../../nsm", "-I../../nsm/L2", "-I../../nsm/L2/garp", "-I../../nsm/L2/gvrp", "-I../../nsm/L2/igmp_snoop", "-I../../nsm", "-I../../nsm/L2", "-I../../nsm/rib", "-I../../imi", "-o", "/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion-build/platform/linux/obj/pal/imi/pal_atl_atmf.o", "imi/pal_atl_atmf.c"),
    ("powerpc-603e-linux-uclibc-gcc", "-DHAVE_CONFIG_H", "-I.", "-I../../openatlibs/cntrd", "-I..", "-I.", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/include/glib-2.0", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/lib/glib-2.0/include", "-Wall", "-Werror", "--sysroot=/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging", "-L/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/lib", "-Os", "-g2", "-mcpu=603e", "-DATL_CHANGE", "-MT", "cntrd_app_gen.lo", "-MD", "-MP", "-MF", ".deps/cntrd_app_gen.Tpo", "-c", "../../openatlibs/cntrd/cntrd_app_gen.c", "-o", "cntrd_app_gen.o"),
)

class TestTraceExec(unittest.TestCase):
    """
    Requires directory containing 'libtrace-exec.so' to be in LD_LIBRARY_PATH

    The Makefile can do this for you and run these tests:

        $ make test

    """
    def setUp(self):
        self.trace_exec = ctypes.CDLL('libtrace-exec.so')

    def join_args(self, args):
        self.trace_exec.shell_quote.restype = ctypes.c_char_p
        shell_quote = self.trace_exec.shell_quote
        return ' '.join((shell_quote(arg) for arg in args))


    def test_shell_quote(self):
        cases = (
            ('-DSOMEDEF=A spaced string.', '-DSOMEDEF=A\\ spaced\\ string.'),
            ('|', '\\|'),
            ('CHEE$E', 'CHEE\\$E')
        
        )
        self.trace_exec.shell_quote.restype = ctypes.c_char_p
        for case in cases:
            arg = ctypes.c_char_p(case[0])
            
            quoted = case[1]
            quoted2 = self.trace_exec.shell_quote(arg)

            self.assertEqual(quoted, quoted2)


    def test_trace_path_remove(self):
        cases = (
            {'message': 'None',
             'before': '/usr/bin:/bin:/usr/share/local/bin',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/nonsense'},
            {'message': 'One leading',
             'before': '/nonsense:/usr/bin:/bin:/usr/share/local/bin',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/nonsense'},
            {'message': 'One leading, one middle',
             'before': '/nonsense:/usr/bin:/nonsense:/bin:/usr/share/local/bin',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/nonsense'},
            {'message': 'One middle',
             'before': '/usr/bin:/bin:/path/to/bin:/usr/share/local/bin',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/path/to/bin'},
            {'message': 'One trailing',
             'before': '/usr/bin:/a/path:/bin:/a/path:/a/path:/usr/share/local/bin',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/a/path'},
            {'message': 'One trailing',
             'before': '/usr/bin:/bin:/usr/share/local/bin:/nonsense',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/nonsense'},
            {'message': 'One trailing, one middle',
             'before': '/usr/bin:/bin:/nonsense:/usr/share/local/bin:/nonsense',
             'after': '/usr/bin:/bin:/usr/share/local/bin',
             'remove': '/nonsense'},
        )
        self.trace_exec.trace_path_remove.restype = ctypes.c_char_p
        for case in cases:
            path = ctypes.create_string_buffer(case['before'])
            remove_path = ctypes.c_char_p(case['remove'])

            self.trace_exec.trace_path_remove(path, remove_path)
            self.assertEqual(
                path.value, case['after'],
                '%s: Got: "%s", Expected: "%s"' %
                (case['message'], path.value, case['after'])
            )

    def test_trace_get_package(self):
        cases = (
            'magical-arcane-package',
            'package with spe&^&&^%&^%&^%@2\xFF\x45 chars',
            'ipinfusion',
        )
        self.trace_exec.trace_get_package.restype = ctypes.c_char_p
        for package_name in cases:
            os.putenv("BS_PACKAGE_NAME", package_name)
            package_name2 = self.trace_exec.trace_get_package()
            self.assertEqual(package_name, package_name2)
            # FIXME: Test leaks package_name2

    def test_trace_get_directory(self):
        self.trace_exec.trace_get_directory.restype = ctypes.c_char_p

        cwd = os.getcwd()
        cwd2 = self.trace_exec.trace_get_directory()
        self.assertEqual(cwd, cwd2)
        # FIXME: Test leaks cwd2

    def test_trace_get_command(self):
        self.trace_exec.trace_get_command.restype = ctypes.c_char_p
        for case in ARG_CASES:
            argc = len(case)
            ArgvArr = (ctypes.c_char_p * argc)
            argv = ArgvArr(*(ctypes.c_char_p(arg) for arg in case))

            cmd = self.join_args(case)
            cmd2 = self.trace_exec.trace_get_command(argc, argv)
            self.assertEqual(cmd, cmd2)
            # FIXME: Test leaks every cmd2

    def test_trace_send(self):
        for case in ARG_CASES:
            argc = len(case)
            ArgvArr = (ctypes.c_char_p * argc)
            argv = ArgvArr(*(ctypes.c_char_p(arg) for arg in case))
            os.putenv("BS_PACKAGE_NAME", "a-package")
            with tempfile.TemporaryFile() as f:
                sockfd = f.fileno()
                self.trace_exec.trace_send(sockfd, argc, argv)
                f.seek(0)
                trace2 = f.read()
            trace = '%s\t%s\t%s' % ('a-package',
                                    os.getcwd(),
                                    self.join_args(case),)
            self.assertEqual(trace, trace2)

    def test_trace_transport_unix(self):
        self.trace_exec.trace_transport_unix.restype = ctypes.c_int

        path = "/tmp/un_%s.sock" % random.randint(0, 1024)

        def listener(pipe):
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.bind(path)
            sock.listen(1)
            conn, addr = sock.accept()
            while 1:
                data = conn.recv(1024)
                if not data:
                    break
                os.write(pipe, data)
            os.close(pipe)

        r_pipe, w_pipe = os.pipe()
        reflector = threading.Thread(target=listener, args=(w_pipe,))
        reflector.start()

        # Wait for the listener to bind 
        time.sleep(0.01)

        os.putenv("TRACE_UNIX", path)
        sock2 = self.trace_exec.trace_transport_unix()

        message = "Hello World!"
        os.write(sock2, message)
        self.trace_exec.trace_transport_close(sock2)

        buf = cStringIO.StringIO()
        while 1:
            data = os.read(r_pipe, 1024)
            if not data:
                break
            buf.write(data)
        buf.seek(0)
        message2 = buf.read()

        self.assertEqual(message, message2)

    #TODO Tidy up test_trace_transport_inet and test_trace_transport_unix
    #     as they are messy, and share alot of common code, maybe they
    #     should be broken out into their own TestCase class.

    def test_trace_transport_inet(self):
        self.trace_exec.trace_transport_inet.restype = ctypes.c_int

        hostname = "localhost"
        port = random.randint(40000, 48000)

        def listener(pipe):
            addr = (socket.gethostbyname(hostname), port)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.bind(addr)
            sock.listen(1)
            conn, addr = sock.accept()
            while 1:
                data = conn.recv(1024)
                if not data:
                    break
                os.write(pipe, data)
            os.close(pipe)

        r_pipe, w_pipe = os.pipe()
        reflector = threading.Thread(target=listener, args=(w_pipe,))
        reflector.start()

        # Wait for the listener to bind 
        time.sleep(0.01)

        os.putenv("TRACE_HOST", hostname)
        os.putenv("TRACE_PORT", str(port))
        sock2 = self.trace_exec.trace_transport_inet()

        message = "Hello World!"
        os.write(sock2, message)
        self.trace_exec.trace_transport_close(sock2)

        buf = cStringIO.StringIO()
        while 1:
            data = os.read(r_pipe, 1024)
            if not data:
                break
            buf.write(data)
        buf.seek(0)
        message2 = buf.read()

        self.assertEqual(message, message2)

if __name__ == '__main__':
    unittest.main()
