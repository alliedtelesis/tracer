import ctypes
import unittest
import os


class TestTraceExecFunctions(unittest.TestCase):
    """
    Requires directory containing 'libtrace-exec.so' to be in LD_LIBRARY_PATH
    """
    def setUp(self):
        self.trace_exec = ctypes.CDLL('libtrace-exec.so')
        self.trace_exec.env_path_remove.restype = ctypes.c_char_p

    def test_env_path_remove(self):
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
        for case in cases:
            path = ctypes.create_string_buffer(case['before'])
            remove_path = ctypes.c_char_p(case['remove'])

            self.trace_exec.env_path_remove(path, remove_path)
            self.assertEqual(
                path.value, case['after'],
                '%s: Got: "%s", Expected: "%s"' %
                (case['message'], path.value, case['after'])
            )

if __name__ == '__main__':
    unittest.main()
