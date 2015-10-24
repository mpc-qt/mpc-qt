#!/usr/bin/env python3
from subprocess import Popen,call,PIPE
from string import Template
from os import getcwd, walk, path

boilerplate = Template('''[Desktop Entry]
Comment=
Exec=env LD_PRELOAD=$libmpv $mpc
GenericName=
Icon=$icon
MimeType=
Name=Media Player Classic Qute Theater
Path=
StartupNotify=true
Terminal=false
TerminalOptions=
Type=Application
X-DBUS-ServiceName=
X-DBUS-StartupType=
X-KDE-SubstituteUID=false
X-KDE-Username=
''')

print('Searching for libmpv binary.')
search = Popen(['whereis', 'libmpv.so'], stdout=PIPE)
libmpvs = search.communicate()[0].decode('utf-8').split(' ')
if len(libmpvs) < 2:
  print('Cannot find libmpv binary. Aborting')
  exit(1)
libmpv = libmpvs[1].strip('\n')
print('Found libmpv binary at\n\t' + libmpv + '\n')

print('Searching for mpc-qt binary.')
mpc = ''
for subdir, dirs, files in walk('../'):
  if 'build' in subdir and 'mpc-qt' in files:
    mpc = path.abspath(subdir + '/mpc-qt')
if len(mpc) < 1:
  print('Cannot find mpcqt.  Have you compiled it?')
  exit(1)
print('Found mpc-qt binary at\n\t' + mpc + '\n')

icon = path.abspath(getcwd() + '/images/bitmaps/icon.png')
print('Icon appears to be at\n\t' + icon + '\n')

print('Information gathered, creating desktop entry')
with open('Media Player Classic Qute Theater.desktop', 'w') as f:
  f.write(boilerplate.safe_substitute(libmpv=libmpv, mpc=mpc, icon=icon))
