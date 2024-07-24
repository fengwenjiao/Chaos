# -*- coding: utf-8 -*-
import os
import sys


def find_lib_path(prefix="libconstellation"):
    """Find constellation library path.
        :param:
        prefix : str, default 'libconstellation'
            The prefix of the library name.
        :returns: list
            A list of library path.
        :raises: RuntimeError
            If no library is found.
     """
    curr_path = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
    install_path = [os.path.join(sys.prefix, "constellation")]
    api_path = [os.path.join(curr_path, "../../lib"), os.path.join(curr_path, "constellation")]
    cmake_path = [os.path.join(curr_path, "../../build"), os.path.join(curr_path, "../../cmake-build-debug")]

    lib_path = [curr_path] + api_path + cmake_path + install_path
    lib_path = [os.path.join(p, prefix + ".so") for p in lib_path]
    lib_path_res = [p for p in lib_path if os.path.exists(p) and os.path.isfile(p)]

    if len(lib_path_res) == 0:
        raise RuntimeError("Cannot find library\n"
                           "Candidates include:\n" + "\n".join(lib_path))
    return lib_path_res


__version__ = "0.1.0"
