from __future__ import print_function

import argparse
import glob
import sys
import os

prefixes = ['src', '../src', '_gen_src']

def process_all_headers(outfile, processed=set()):
    flist = []
    for prefix in prefixes:
        flist += glob.glob(prefix + '/**/*.h', recursive=True)
    for fname in flist:
        process_header(fname, outfile, processed)

def process_header(fname, outfile, processed=set()):
    relname = fname.split('/', 1)[1]
    processed.add(relname)
    f = open(fname, 'r')
    for line in f.readlines():
        if line[0:8] == '#include':
            header = line[9:].strip('\r\n\t \"<>')
            if header[0:6] == 'photon':
                if not header in processed:
                    for prefix in prefixes:
                        fname = prefix + '/' + header
                        if os.path.isfile(fname):
                            process_header(fname, outfile, processed)
            else:
                outfile.write(line)
        else:
            outfile.write(line)
    outfile.write('\n')

def process_all_sources(outfile, header_name='photon.h'):
    outfile.write('#include \"' + header_name + '\"\n')
    plist = []
    for prefix in prefixes:
        for fname in glob.glob(prefix + '/**/*Private.inc.c', recursive=True):
            plist.append(fname)
            f = open(fname, 'r')
            outfile.write(f.read())
            outfile.write('\n')
    for prefix in prefixes:
        for fname in glob.glob(prefix + '/**/*.c', recursive=True):
            if fname not in plist:
                f = open(fname, 'r')
                for line in f.readlines():
                    if line[0:8] == '#include':
                        header = line[9:].strip('\r\n\t \"<>')
                        if not header[0:6] == 'photon':
                            outfile.write(line)
                    else:
                        outfile.write(line)
    outfile.write('\n')

if not os.path.exists('amalgamation'):
    os.makedirs('amalgamation')
process_all_headers(open('amalgamation/photon.h', 'w'))
process_all_sources(open('amalgamation/photon.c', 'w'))
