#!/usr/bin/env python3
"""Platform adaptation tests of the MB-System command line programs.

These run on every platform. They check the behaviour that the native
Windows port must reproduce: byte-identical output (no CRLF translation),
file names with spaces and non-ASCII characters, Windows-style paths in
datalists, the home directory fallback and data file discovery.
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE_DIR = os.environ.get('MB_TEST_SOURCE_DIR',
                            os.path.join(os.path.dirname(__file__), '..', '..'))
BINARY_DIR = os.environ.get('MB_TEST_BINARY_DIR', os.path.join('..', '..'))
IS_WINDOWS = sys.platform.startswith('win')

TESTDATA = os.path.join(SOURCE_DIR, 'test', 'utilities', 'testdata', 'mb21',
                        'TN136HS.309.snipped.mb21')


def program(name):
  path = os.path.join(BINARY_DIR, 'src', 'utilities', name)
  if IS_WINDOWS:
    path += '.exe'
  return path


def run(args, env=None, cwd=None):
  return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        env=env, cwd=cwd, check=False)


class PlatformCliTest(unittest.TestCase):

  def setUp(self):
    self.tmpdir = tempfile.mkdtemp(prefix='mb_platform_')

  def tearDown(self):
    shutil.rmtree(self.tmpdir, ignore_errors=True)

  def info_lines(self, path):
    proc = run([program('mbinfo'), '-I' + path])
    self.assertEqual(0, proc.returncode, proc.stderr)
    # drop the line naming the file, everything else must match
    return [l for l in proc.stdout.split(b'\n') if not l.startswith(b'Swath Data File')]

  def testOutputHasUnixLineEndings(self):
    proc = run([program('mbinfo'), '-I' + TESTDATA])
    self.assertEqual(0, proc.returncode, proc.stderr)
    self.assertIn(b'\n', proc.stdout)
    self.assertNotIn(b'\r\n', proc.stdout)
    proc = run([program('mbinfo'), '-h'])
    self.assertNotIn(b'\r\n', proc.stdout + proc.stderr)

  def testPathsWithSpacesAndNonAsciiCharacters(self):
    # "测线 数据" = "survey line data"
    target_dir = os.path.join(self.tmpdir, '测线 数据')
    os.makedirs(target_dir)
    target = os.path.join(target_dir, 'line 001.mb21')
    shutil.copyfile(TESTDATA, target)
    self.assertEqual(self.info_lines(TESTDATA), self.info_lines(target))

  def testDatalistWithNativePaths(self):
    target = os.path.join(self.tmpdir, 'line_001.mb21')
    shutil.copyfile(TESTDATA, target)
    datalist = os.path.join(self.tmpdir, 'datalist.mb-1')
    with open(datalist, 'w') as f:
      # os.path.abspath gives backslashes and a drive letter on Windows
      f.write('%s 21\n' % os.path.abspath(target))
    proc = run([program('mbinfo'), '-F-1', '-I' + datalist])
    self.assertEqual(0, proc.returncode, proc.stderr)
    self.assertIn(b'Number of Records:                           2', proc.stdout)

  def testRelativeDatalistEntries(self):
    sub = os.path.join(self.tmpdir, 'survey')
    os.makedirs(sub)
    shutil.copyfile(TESTDATA, os.path.join(sub, 'line_001.mb21'))
    datalist = os.path.join(self.tmpdir, 'datalist.mb-1')
    with open(datalist, 'w') as f:
      f.write('survey%sline_001.mb21 21\n' % os.sep)
    proc = run([program('mbinfo'), '-F-1', '-I' + datalist])
    self.assertEqual(0, proc.returncode, proc.stderr)
    self.assertIn(b'Number of Records:                           2', proc.stdout)

  def testDefaultsFileUsesHomeDirectoryFallback(self):
    env = dict(os.environ)
    env.pop('HOME', None)
    if IS_WINDOWS:
      env['USERPROFILE'] = self.tmpdir
    else:
      env['HOME'] = self.tmpdir
    proc = run([program('mbdefaults'), '-L1'], env=env)
    self.assertEqual(0, proc.returncode, proc.stderr)
    defaults = os.path.join(self.tmpdir, '.mbio_defaults')
    self.assertTrue(os.path.exists(defaults))
    with open(defaults) as f:
      self.assertIn('lonflip:    1', f.read())
    proc = run([program('mbdefaults')], env=env)
    self.assertEqual(0, proc.returncode, proc.stderr)
    self.assertIn(b'lonflip:    1', proc.stdout + proc.stderr)

  def testLevitusDatabaseFromShareDirectory(self):
    env = dict(os.environ)
    env['MBSYSTEM_SHAREDIR'] = os.path.join(SOURCE_DIR, 'src', 'share')
    out = os.path.join(self.tmpdir, 'velocity profile.svp')
    proc = run([program('mblevitus'), '-R-130/30', '-O' + out], env=env)
    self.assertEqual(0, proc.returncode, proc.stderr)
    with open(out, 'rb') as f:
      lines = [l for l in f.read().split(b'\n') if l and not l.startswith(b'#')]
    self.assertGreater(len(lines), 10)
    depth, velocity = (float(v) for v in lines[0].split()[:2])
    self.assertEqual(0.0, depth)
    self.assertTrue(1400.0 < velocity < 1600.0, velocity)


if __name__ == '__main__':
  unittest.main()
