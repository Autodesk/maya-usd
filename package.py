name = 'AL_USDMaya'

version = '0.0.1'

uuid = 'c1c2376f-3640-4046-b55e-f11461431f34'

authors = ['AnimalLogic']

description = 'USD to Maya translator. This rez package is purely an example and should be modifyed to your own needs'

private_build_requires = [
    'cmake-2.8+',
    'gcc-4.8',
    'gdb-7.10'
]

requires = [
    'usd-0.7',
    'usdImaging-0.7',
    'glew-2.0',
    'python-2.7+<3',
    'doubleConversion-1',
    'stdlib-4.8',
    'zlib-1.2',
    'googletest',
]

variants = [
    ['CentOS-6.2+<7']
]

def commands():
    prependenv('PATH', '{root}/src')
    prependenv('PYTHONPATH', '{root}/lib/python')
    prependenv('LD_LIBRARY_PATH', '{root}/lib')
    prependenv('MAYA_PLUG_IN_PATH', '{root}/plugin')
    prependenv('MAYA_SCRIPT_PATH', '{root}/lib:{root}/share/usd/plugins/usdMaya/resources')
    prependenv('PXR_PLUGINPATH', '{root}/share/usd/plugins')
    prependenv('CMAKE_MODULE_PATH', '{root}/cmake')
