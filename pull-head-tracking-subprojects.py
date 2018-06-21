#!/usr/bin/env python

from __future__ import print_function

import io
import os
import sys
import subprocess

if not os.path.exists('subprojects'):
    print('No suprojects dir found')
    sys.exit(1)

wraps = os.listdir('subprojects')

enc = sys.stdout.encoding
if enc is None:
    import locale
    enc = locale.getpreferredencoding()

def print_popen(popen):
    out, err = popen
    if err is not None:
        sys.stdout.write(err.decode(enc))
    if out is not None:
        sys.stdout.write(out.decode(enc))


for name in wraps:
    if not name.endswith('.wrap'):
        continue
    path = os.path.join('subprojects', name)

    if not os.path.exists(path):
        print('Path not found:', path)
        sys.exit(1)
    f = open(path, 'r')
    header = f.readline().strip()
    if header != '[wrap-git]':
        continue
    values = dict()
    for line in f:
        line = line.strip()
        if line == '':
            continue
        (k, v) = line.split('=', 1)
        k = k.strip()
        v = v.strip()
        values[k] = v
    d = values['directory']
    if d is None:
        continue
    rev = values['revision']
    path = os.path.join('subprojects', d)
    if not os.path.exists(path):
        print('Path not found:', path)
        sys.exit(1)
    out = io.StringIO()
    print('> Updating', path + '...')
    if rev == 'head':
        popen = subprocess.Popen(['git', 'pull'], cwd=path, stdout=subprocess.PIPE)
        print_popen(popen.communicate())
    else:
        popen = subprocess.Popen(['git', 'rev-parse', 'HEAD'], cwd=path, stdout=subprocess.PIPE)
        out, err = popen.communicate()
        if out.decode(enc).strip() != rev:
            popen = subprocess.Popen(['git', 'fetch'], cwd=path, stdout=subprocess.PIPE)
            print_popen(popen.communicate())
            popen = subprocess.Popen(['git', 'checkout', rev], cwd=path, stdout=subprocess.PIPE)
            print_popen(popen.communicate())
        else:
            print('Already up to date.')
