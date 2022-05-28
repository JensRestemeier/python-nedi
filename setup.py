"""A setuptools based setup module.
See:
https://packaging.python.org/en/latest/distributing.html
https://github.com/pypa/sampleproject
"""

# Always prefer setuptools over distutils
from setuptools import setup, Extension
from os import path

module1 = Extension('_nedi',
                    sources = ['nedi.cpp'],
                    include_dirs = ["Eigen"])

setup(
    name='nedi',

    # Versions should comply with PEP440.  For a discussion on single-sourcing
    # the version across setup.py and the project code, see
    # https://packaging.python.org/en/latest/single_source_version.html
    version='0.0.1',

    description='nedi plugin',
    long_description="",

    # The project's main homepage.
    url='https://github.com/JensRestemeier/python-nedi',

    # Author details
    author='Jens Ch. Restemeier',
    author_email="jens.restemeier@gmail.com",

    # Choose your license
    license='MIT',

    install_requires=[
        'pillow',  
    ],

    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 3 - Alpha',

        # Indicate who your project is intended for
        'Intended Audience :: Developers',

        # Pick your license as you wish (should match "license" above)
        'License :: OSI Approved :: MIT License',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX :: Linux',
        "Operating System :: MacOS",
        "Operating System :: OS Independent",
        
        'Topic :: Software Development :: Libraries'
        
    ],

    keywords=['nedi'],

    packages=['nedi'],

    platforms = ["Windows", "Linux", "MacOS"],
    
    include_package_data=True,

    ext_modules = [module1],
)
