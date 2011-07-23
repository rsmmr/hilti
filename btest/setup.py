#! /usr/bin/env python

from distutils.core import setup

setup(name='btest',
      version="0.22",
      description='A simple unit testing framework',
      author='Robin Sommer',
      author_email='robin@icir.org',
      url='http://www.icir.org/robin/btest',
      scripts=['btest', 'btest-diff', "btest-bg-run", "btest-bg-run-helper", "btest-bg-wait", "btest-setsid"],
      include_dirs=["examples", "Baseline"]
     )
