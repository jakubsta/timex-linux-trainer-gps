from distutils.core import setup, Extension

module1 = Extension('timex', sources = ['timex_wrapper.c', 'timex.c', 'timex_utils.c'])

setup (name = 'Timex', version = '1.0', description = 'This is timex reader', ext_modules = [module1])
