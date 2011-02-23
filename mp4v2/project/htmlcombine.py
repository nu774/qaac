#!/usr/bin/env python3.0
#

import re
import sys

from optparse import OptionParser

###############################################################################

parser = OptionParser( 'Usage: %prog [OPTIONS]' )
parser.add_option( '--header', action='store', default=None, help='header file' )
parser.add_option( '--footer', action='store', default=None, help='footer file' )
parser.add_option( '--body', action='store', default=None, help='file to extract contents of body from' )
parser.add_option( '-v', '--verbose', action='count', default=False, help='increase verbosity' )

(options, args) = parser.parse_args()

if( len(args) != 0 ):
    parser.error( 'incorrect number of arguments' )

if not options.header:
    parser.error( '--header must be specified' )
if not options.footer:
    parser.error( '--footer must be specified' )
if not options.body:
    parser.error( '--body must be specified' )

###############################################################################

title = 'Unknown'

# parse majorheading
with open( options.body, 'r' ) as f:
    rx = re.compile( '^.*class="majorheading".*>([^>]+)</.+>' )
    for line in f:
        m = rx.match( line )
        if not m:
            continue;
        title = m.group( 1 )
        break

m = re.compile( '(\S+)\s+(\S+)\s+(.+)' ).match( title )
if m:
    shortTitle = m.group(3)
else:
    shortTitle = title

menu = ''
if not shortTitle == 'Documentation':
    menu += '<li><a href="index.html">Documentation</a></li>\n'
    menu += '<li class="active">%s</li>' % (shortTitle)
else:
    menu += '<li class="active">Documentation</li>'

with open( options.header, 'r' ) as f:
    rxTitle      = re.compile( '__TITLE__' )
    rxShortTitle = re.compile( '__SHORT_TITLE__' )
    rxMenu       = re.compile( '__MENU__' )
    for line in f:
        line = re.sub( rxTitle, title, line )
        line = re.sub( rxShortTitle, shortTitle, line )
        line = re.sub( rxMenu, menu, line )
        sys.stdout.write( line )

# out everything *after* first <body> and *before* </body>
with open( options.body, 'r' ) as f:
    rxBegin    = re.compile( '<body>' )
    rxEnd      = re.compile( '</body>' )
    inside  = False
    lstrip  = False
    for line in f:
        if rxBegin.match( line ):
            inside = True
            continue
        if rxEnd.match( line ):
            outside = False
            continue

        if lstrip:
            line = line.lstrip()
        if inside:
            sys.stdout.write( line )

with open( options.footer, 'r' ) as f:
    for line in f:
        sys.stdout.write( line )
