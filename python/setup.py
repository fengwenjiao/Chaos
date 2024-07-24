from setuptools import setup, find_packages
import os
import sys

CURRENT_DIR = os.path.dirname(__file__)

lib_path_py = os.path.join(CURRENT_DIR, 'constellation/find_lib_path.py')
lib_info = {'__file__': lib_path_py}
exec(compile(open(lib_path_py, "rb").read(), lib_path_py, 'exec'), lib_info, lib_info)
sys.path.insert(0, CURRENT_DIR)

LIB_PATH = lib_info['find_lib_path']()
__version__ = lib_info['__version__']


setup(
    packages=find_packages(),
    data_files=[("constellation", [LIB_PATH[0]])],
)
