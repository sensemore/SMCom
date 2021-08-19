##### Sensemore Communication Protocol : SMCom  #######
#	

#Required packages
# - pybind11
# - pyserial


import os,sys
import subprocess
import pathlib
from distutils import spawn
from setuptools import setup, find_packages, Extension

from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir


# c++ -shared -fPIC $(python3 -m pybind11 --includes) config.cpp SMCom.cpp -o SMCom$(python3-config --extension-suffix)

file_abs_path = os.path.abspath(__file__)
smcompy_directory = os.path.dirname(file_abs_path)
repo_directory = os.path.dirname(smcompy_directory)

so_flags = ["-shared", "-fPIC" ]
module_name = "SMComPy"

source_files = [
	os.path.join(repo_directory,"src/SMCom.cpp"),
	os.path.join(smcompy_directory,"pybind11_config.cpp")
]

header_files = [
	os.path.join(repo_directory,"include")
]

optimization_flag = "-O0"

def generate_so_file():
	compiler = spawn.find_executable("g++")

	p = subprocess.run(["python3","-m", "pybind11", "--includes"],capture_output=True)
	pybind11_includes = str(p.stdout,'utf-8').strip('\n').split()

	p = subprocess.run(["python3-config","--extension-suffix"],capture_output=True)
	python_extension = str(p.stdout,'utf-8').strip('\n')

	subprocess.run([compiler,
					optimization_flag,
					*so_flags,
					*pybind11_includes,
					"-I",
					*header_files,
					*source_files,
					"-o",
					module_name + python_extension
				])

ext_modules = [
    Pybind11Extension(module_name,
        sources = ["src/SMCom.cpp", "SMComPy_src/pybind11_config.cpp"],
		include_dirs = ["include"],
		language="c++"
	),
]

setup(
    name=module_name,
    version="0.0.1",
    author="sensemore",
    url="https://www.sensemore.io",
    description="SMComPy project",
    long_description="",
    ext_modules=ext_modules,
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)