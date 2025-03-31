#!/usr/bin/python3

##############################
# Select Linux dependencies for inclusion into AppImage (`ldd` command)
##############################

from urllib.request import urlopen
import subprocess
import sys
import re
import os

__WHITELIST = [
    'libnsl',
    'libSDL2',
    'libSDL2_mixer',
    'libgme',
    'libopenmpt',
    'libpng',
    'libfluidsynth',
    'libpulse',
    'libmodplug',
    'libvorbisfile',
    'libopusfile',
    'libFLAC',
    'libmad',
    'libmpg123',
    'libvorbis',
    'libpulsecommon',
    'libsndfile',
    'libsndio',
    'libogg',
    'libvorbisenc',
    'libjson',
    'libreadline',
    'libwrap',
    'libtinfo',
    'libtirpc',
    'libpng16'
]

if len(sys.argv) < 2:
    raise ValueError("First argument must be a path to the program executable.")

#print('Whitelist: Matching libraries against {} names. See {} for the list.'.format(len(__WHITELIST), os.path.basename(__file__)))

# Get most recent excludelist for AppImage depends
#with urlopen('https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist') as f:
#    excludelist = f.read().decode('utf-8')

# Get only the filenames from excludelist
#exlist = re.sub(r'#.*', '', excludelist)
#exlist = re.sub(r'\s.*$', '', exlist)
#exlist = [s for s in exlist.splitlines() if s]

# Get list of dependencies for EXE name
ldd = subprocess.check_output(['ldd',sys.argv[1]]).decode('utf-8')
ldd = re.sub(r' \(.*', '', ldd)
ldd = ldd.splitlines()

# Format list of dependencies
deps = []
for line in ldd:
    keyval = line.split(' => ')
    if len(keyval) < 2:
        continue

    # Perform basename because a depend may have a path prefix
    #if os.path.basename(keyval[0]) not in exlist:
    #if any([(os.path.basename(keyval[0]).find(s) > -1): for s in __WHITELIST]):
    deps.append(keyval[1])

print(' '.join(deps))
