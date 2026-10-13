#!/usr/bin/env python3
"""Tests of the mbstudio host executable in its command line modes.

They demonstrate the microkernel design: the kernel and the MB-System
plugin work without the graphical front end, which is just another plugin.
"""

import json
import os
import shutil
import subprocess
import tempfile
import unittest

EXE = os.environ.get('MBSTUDIO_EXE', 'mbstudio')
SOURCE_DIR = os.environ.get('MB_TEST_SOURCE_DIR', os.path.join('..', '..', '..', '..'))
TESTDATA = os.path.join(SOURCE_DIR, 'test', 'utilities', 'testdata', 'mb21',
                        'TN136HS.309.snipped.mb21')


def run(*args):
  proc = subprocess.run([EXE] + list(args), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        timeout=300, check=False)
  return proc.returncode, proc.stdout.decode('utf-8'), proc.stderr.decode('utf-8')


class HostTest(unittest.TestCase):

  def testVersion(self):
    code, out, _ = run('--version')
    self.assertEqual(0, code)
    self.assertIn('MB-Studio 0.', out)

  def testPluginsAreDiscovered(self):
    code, out, err = run('--list-plugins')
    self.assertEqual(0, code, err)
    lines = {l.split()[0]: l.split()[1] for l in out.splitlines() if l.strip()}
    self.assertEqual('active', lines.get('mbsystem'), out)

  def testServicesAndCommandsAreRegistered(self):
    code, out, _ = run('--headless', '--list-services', '--list-commands')
    self.assertEqual(0, code)
    for name in ('mbsystem.swath', 'mbsystem.tools', 'swath.inspect', 'tools.run'):
      self.assertIn(name, out)
    self.assertNotIn('mbstudio.frontend', out)  # the UI plugin is not started headless

  def testSwathInspectCommand(self):
    code, out, err = run('--command', 'swath.inspect', 'path=' + TESTDATA)
    self.assertEqual(0, code, out + err)
    result = json.loads(out)
    self.assertTrue(result['ok'])
    self.assertEqual(2, result['records'])
    self.assertEqual('MBF_HSATLRAW', result['formatName'])

  def testToolsRunCommand(self):
    code, out, err = run('--command', 'tools.run', 'tool=mbinfo', 'args=-I"%s"' % TESTDATA)
    self.assertEqual(0, code, out + err)
    result = json.loads(out)
    self.assertEqual(0, result['exitCode'])
    self.assertIn('MBIO Data Format ID:  21', result['output'])

  def testHelpProgram(self):
    code, out, err = run('--help-program', 'mbinfo', '--lang', 'en')
    self.assertEqual(0, code, err)
    self.assertTrue(out.startswith('# mbinfo '), out[:200])
    code, out, err = run('--help-program', 'mbgrid', '--lang', 'ja')
    self.assertEqual(0, code, err)
    self.assertIn('## 書式', out)
    code, out, err = run('--help-program', 'mbgrid', '--lang', 'zh')
    self.assertEqual(0, code, err)
    self.assertIn('## 语法', out)
    code, _, err = run('--help-program', 'no-such-program')
    self.assertEqual(1, code)
    self.assertIn('no-such-program', err)

  def testHelpCommands(self):
    code, out, err = run('--command', 'help.programs')
    self.assertEqual(0, code, err)
    result = json.loads(out)
    self.assertEqual('en,ja,zh_CN', result['languages'])
    self.assertIn('program.mbgrid', result)

  def testProjectCommands(self):
    # the project plugin needs Qt Core; skip where it is not built
    _, plugins, _ = run('--list-plugins')
    if 'project' not in plugins:
      self.skipTest('project plugin not available')
    root = tempfile.mkdtemp(prefix='mbstudio project ')
    try:
      project = os.path.join(root, 'survey')
      raw = os.path.join(project, 'raw')
      code, out, err = run('--command', 'project.create', 'dir=' + project, 'name=Survey')
      self.assertEqual(0, code, out + err)
      self.assertTrue(json.loads(out)['file'].endswith('.mbsproj'))
      os.makedirs(raw)
      shutil.copy(TESTDATA, raw)
      code, out, err = run('--command', 'project.add', 'project=' + project, 'path=' + raw)
      self.assertEqual(0, code, out + err)
      result = json.loads(out)
      self.assertEqual(1, result['added'])
      self.assertEqual(1, result['inspected'])
      code, out, err = run('--command', 'project.grid', 'project=' + project, 'spacing=20', 'name=bathy')
      self.assertEqual(0, code, out + err)
      result = json.loads(out)
      self.assertEqual('completed', result['state'], out)
      self.assertGreaterEqual(result['assets'], 1)
      self.assertTrue(os.path.exists(os.path.join(project, 'products', 'grids', 'bathy.grd')))
      code, out, err = run('--command', 'project.info', 'project=' + project)
      self.assertEqual(0, code, out + err)
      self.assertEqual(2, json.loads(out)['jobs'])
    finally:
      shutil.rmtree(root, ignore_errors=True)

  def testFailingCommand(self):
    code, out, _ = run('--command', 'swath.inspect', 'path=does-not-exist.mb21')
    self.assertEqual(1, code)
    self.assertFalse(json.loads(out)['ok'])

  def testUnknownCommand(self):
    code, out, _ = run('--command', 'no.such.command')
    self.assertEqual(1, code)
    self.assertIn('unknown command', json.loads(out)['message'])


if __name__ == '__main__':
  unittest.main()
