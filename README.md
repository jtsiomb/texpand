texpand
=======

`texpand` is a texture pre-processing tool, meant to be used as part of the
content pipeline for games, or other graphics programs. Its main purpose is to
expand texels to cover all adjacent unused areas in the image, in order to avoid
filtering artifacts during rendering.

![result](http://nuclear.mutantstargoat.com/sw/texpand/img/shot.jpg)

Additionally, `texpand` is also able to produce texture coverage images,
visualizing the unwrapped 3D models in texture space, which can be used as input
for the expand operation, or used as reference for artists to paint the texture
of an unwrapped model.

Finally, `texpand` may also be used to calculate the percentage of used space in
a texture image, which can be useful to issue automated warnings during asset
builds, for inefficient use of texture storage.

License
-------
Copyright (C) 2016 John Tsiombikas <nuclear@member.fsf.org>

This program is free software. You may use, modify, and/or redistribute it,
under the terms of the GNU General Public License version 3, or at your option
any later version published by the Free Software Foundation. See COPYING for
details.

Build instructions
------------------
`texpand` depends on the following libraries, which need to be installed
system-wide before building:

 - imago2: http://github.com/jtsiomb/libimago
 - assimp: http://www.assimp.org

Simply type `make` to build, and optionally `make install` to install `texpand`.
If you don't want to install to the default prefix (which is `/usr/local`),
make sure to modify the first line of the `Makefile`.

Usage
-----
```
Usage: ./texpand [options] <texture file>
Options:
   -o <fname>: specify output filename
   -uvset <n>: specify which UV set to use (default: 0)
   -force, -f: use of all meshes specified without matching the texture filename
   -genmask: output the texture usage mask
   -mesh <fname>: use mesh/scene file for generating the texture usage mask
   -mask <fname>: use a mask file instead of generating it from geometry mesh
   -maskalpha: use alpha channel as the usage mask
   -usage, -u: calculate and print texture space utilization [0, 1]
   -help, -h: print usage information and exit
 (exactly one of -mesh, -mask, or -maskalpha must be specified).
```

Issues
------
Currently `texpand` uses X11/GLX, to create an OpenGL context for building the
mesh coverage mask. So it needs a running X server supporting the GLX extension
to work. The connection is only attempted when building a mask however, so it
doesn't affect the other modes of operation.

Meshes with texture coordinates beyond the interval [0, 1] are clipped.
