import ctypes
import ctypes.util
import unittest
import os


class TestTraceExecFunctions(unittest.TestCase):
    """
    Requires directory containing 'libtrace-exec.so' to be in LD_LIBRARY_PATH
    """
    def setUp(self):
        self.trace_exec = ctypes.CDLL('libtrace-exec.so')

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
        self.trace_exec.trace_get_package.restype = ctypes.c_char_p

        package_name = "magical-arcane-package"
        os.putenv("PACKAGE_NAME", package_name)
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
        cases = (
            ("executable",),
            ("executable", "arg1", "arg2"),
            ("executable", "longerargumentwithspe^&cialchars"),
            ("gcc", "-Wall", "-c", "file.c", "-o", "file.o"),
            # Line breaks? nuh
            ("powerpc-603e-linux-uclibc-gcc", "-c", "-isystem/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/include",  "-I../..", "-I../../pal/api", "-I../../pal/linux", "-I../../lib", "-I../../cal", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion/platform/linux", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion-build/platform/linux", "--sysroot=/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/new/install/include/", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/hslhw", "-Wformat=2", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/glib-2.0", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/lib/glib-2.0/include", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/staging/usr/include/openhpi", "-Os", "-g2", "-mcpu=603e", "-DATL_CHANGE", "-Wall", "-funsigned-char", "-fno-signed-char", "-fno-strict-aliasing", "-Werror", "-Wno-pointer-sign", "-Wlong-long", "-Wnested-externs", "-I../../lib/fm", "-I../../lib/memmgr", "-I../../hal/PBR", "-I../../imi/util", "-I../../hal", "-I../../nsm", "-I../../nsm/L2", "-I../../nsm/L2/garp", "-I../../nsm/L2/gvrp", "-I../../nsm/L2/igmp_snoop", "-I../../nsm", "-I../../nsm/L2", "-I../../nsm/rib", "-I../../imi", "-o", "/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/ipinfusion/work/ipinfusion-build/platform/linux/obj/pal/imi/pal_atl_atmf.o", "imi/pal_atl_atmf.c"),
            ("powerpc-603e-linux-uclibc-gcc", "-DHAVE_CONFIG_H", "-I.", "-I../../openatlibs/cntrd", "-I..", "-I.", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/include/glib-2.0", "-I/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/lib/glib-2.0/include", "-Wall", "-Werror", "--sysroot=/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging", "-L/home/andrewm/source/work/VCN_1628/buildsys/output/x600-uclibc/openatlibs/staging/usr/lib", "-Os", "-g2", "-mcpu=603e", "-DATL_CHANGE", "-MT", "cntrd_app_gen.lo", "-MD", "-MP", "-MF", ".deps/cntrd_app_gen.Tpo", "-c", "../../openatlibs/cntrd/cntrd_app_gen.c", "-o", "cntrd_app_gen.o"),
        )
        self.trace_exec.trace_get_command.restype = ctypes.c_char_p

        for case in cases:
            argc = len(case)
            ArgvArr = (ctypes.c_char_p * argc)
            argv = ArgvArr(*(ctypes.c_char_p(arg) for arg in case))

            cmd = ' '.join(case)
            cmd2 = self.trace_exec.trace_get_command(argc, argv)
            self.assertEqual(cmd, cmd2)
            # FIXME: Test leaks every cmd2

if __name__ == '__main__':
    unittest.main()
