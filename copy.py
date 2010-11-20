#!/usr/bin/env python

import sys
import os
import shutil

basedir='D:/src/mp4v2-trunk'
platformdir='libplatform'
srcdir='src'

bases = [os.path.join(basedir, x) for x in (platformdir, srcdir)]
for base in bases:
    for root, dirs, files in os.walk(base):
        outdir = root[len(base)+1:]
        if platformdir in root:
            outdir = os.path.join(platformdir, outdir)
        for f in files:
            if f.endswith('.h') or f.endswith('.tcc'):
                try:
                    if outdir:
                        os.makedirs(outdir)
                except OSError as e:
                    if e.errno != 17:
                        raise
                shutil.copyfile(os.path.join(root, f), os.path.join(outdir, f))
