Taisei Project - Compiling FAQ
==============================

.. contents::

Introduction
------------

This document contains some important tips on compiling and developing for
specific platforms.

Basic Build Instructions
------------------------

Taisei's general dependencies are as follows:

-  C (C11) compiler (``gcc``, ``clang``, etc)
-  Python >= 3.5
-  meson >= 0.53.0
-  ninja >= 1.10.0
-  docutils

You can optionally install `other dependencies <../README.rst#dependencies>`__,
but the build system will pull in everything else you might need otherwise.

First, you'll need to checkout the repository. You can do that with the
following:

.. code:: sh

   git clone https://github.com/taisei-project/taisei.git
   cd taisei/
   git submodule update --init --recursive

Ensure that the ``git submodule`` or Taisei will not build, as it will be
missing many of the dependencies its needs to compile.

Next, you'll want to set up a prefix for where the game's binaries will be
installed. For starting out development, you're better off installing it to
your HOME directory.

.. code:: sh

    export TAISEI_PREFIX=/home/{your_username}/bin

To build and install Taisei on \*nix:

.. code:: sh

   meson setup build/ --prefix=$TAISEI_PREFIX -Dwerror=true -Ddeprecation_warnings=no-error
   meson compile -C build/
   meson install -C build/

There are a bunch of different arguments attached to the ``setup`` command.
Here's an explanation of what they do:

- ``--prefix=$TAISEI_PREFIX`` installs Taisei to the prefix you specified in
  the previous step.
- ``-Dwerror=true`` makes compilation **hard fail** on ``Warnings`` put out by
  the compiler. This can be a bit of a pain, but a lot of the time the warnings
  are there for a good reason, so we treat them as errors. As an example, it
  can detect potential null-pointer errors under certain conditions.
- ``-Ddeprecation_warnings=no-error`` turns off those warnings-as-errors for
  deprecation warnings specifically, as some system libraries (such as OpenGL
  on macOS) may have deprecation warnings for things not related to Taisei's
  code.

These commands will set up your build environment, compile the game, and
install it to the ``/home/{your_username}/bin`` directory you specified
earlier. You can then run the game by executing the ``taisei`` binary in that
directory (or running the ``Taisei.app`` for macOS).

Development
-----------

Build Options
"""""""""""""

Game Data Path
''''''''''''''

When compiling with ``TAISEI_PREFIX`` set, game file data will be
installed to ``$TAISEI_PREFIX/share/taisei/``, and this path will be built
*statically* into the executable. You can decide whether or not you want this
based on your own preferences. Alternatively, you can install game data
relatively as well:

.. code:: sh

   meson configure build/ -Dinstall_relative=true

Which will cause save game data to be installed to:

.. code:: sh

    $TAISEI_PREFIX/taisei/
    $TAISEI_PREFIX/data/

Note that ``install relative`` is always set when building for Windows.

Debugging
'''''''''

You can enable debugging options/output for development purposes:

.. code:: sh

    meson configure build/ -Dbuildtype=debug -Db_ndebug=false

Faster Builds
'''''''''''''

This option also helps for speeding up build times, although there is a
theoretical reduction in performance with these options:

.. code:: sh

    meson configure build/ -Db_lto=false -Dstrip=false

Developer Mode
''''''''''''''

For debugging actual gameplay, you can set this option and it will enable cheats
and other 'fast-forward' options by the pressing keys defined in
``src/config.h``.

.. code:: sh

   meson config build/ -Ddeveloper=true

Stack Trace Debugging
'''''''''''''''''''''

This is useful for debugging crashes in the game. It uses
`AddressSanitizer <https://github.com/google/sanitizers/wiki/AddressSanitizer>`__:

.. code:: sh

   meson configure build/ -Db_sanitize=address,undefined

Depending on your platform, you may need to specify the specific library binary
to use to launch ASan appropriately. Using macOS as an example:

.. code:: sh

    export DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/12.0.0/lib/darwin/libclang_rt.asan_osx_dynamic.dylib

The ``../12.0.0/..`` in the path of ``DYLD_INSERT_LIBRARIES`` changes with each
version of XCode. If it fails to launch for you, ensure that the version number
is correct by browsing to the parent directory of ``../clang``.

Then, you can launch Taisei's binary from the command line (using macOS as an
example):

.. code:: sh

    /path/to/Taisei.app/Contents/MacOS/Taisei


OpenGL ES Support (Optional)
""""""""""""""""""""""""""""

3.0
'''

The OpenGL ES 3.0 backend is not built by default. To enable it, do:

.. code:: sh

    meson configure build/ -Dr_gles30=true -Dshader_transpiler=true -Dr_default=gles30

2.0
'''

An experimental OpenGL ES 2.0 backend can be enabled similarly, using:

.. code:: sh

   meson configure build/ -Dr_gles20=true -Dshader_transpiler=true -Dr_default=gles20

However, GLES 2.0 requires a few extensions to be present on your system
to function correctly, most notably:

- ``OES_depth_texture`` or ``GL_ANGLE_depth_texture``
- ``OES_standard_derivatives``
- ``OES_vertex_array_object``
- ``EXT_frag_depth``
- ``EXT_instanced_arrays`` or ``ANGLE_instanced_arrays`` or
  ``NV_instanced_arrays``

ANGLE (Windows/macOS)
'''''''''''''''''''''

ANGLE support is semi-optional for macOS, and *mandatory* for Windows. Due to
long-standing OpenGL bugs on Windows, it's required so that Taisei runs
correctly on as many GPU configurations as possible. macOS runs fine with the
standard OpenGL 3.3 backend, but requires ANGLE for GL ES 3.0 support.

For Windows and macOS, you will need Google's ANGLE library for both ES 3.0 and
2.0. You'll need to check out
`ANGLE <https://github.com/google/angle>`__ and build it first. Refer to their
documentation on how to do that.

Once you've compiled ANGLE, enable it with:

.. code:: sh

    export LIBGLES=/path/to/libGLESv2.{dll,dylib}
    export LIBEGL=/path/to/libEGL.{dll,dylib}
    meson configure build/ -Dinstall_angle=true -Dangle_libgles=$LIBGLES -Dangle_libegl=$LIBEGL

Ensure you use the correct file extension for your platform. (``.dll`` for
Windows, ``.dylib`` for macOS.)

``meson`` will copy those files over into the package itself when packaging with
``ninja zip`` or ``ninja dmg``.

Coding Style
------------

Tabs/Spaces
"""""""""""

In the ``*.c`` files, tabs are used. In the ``meson.build`` and ``*.py`` files,
spaces are used. It's a bit inconsistent, but it's the style that was chosen at
the beginning, and one we're probably going to stick with.

To help you abide by this standard, you should install
`EditorConfig <https://github.com/editorconfig>`__ for your preferred editor of
choice, and load in the file found at ``.editorconfig`` in the root of the
project.

For Loops
"""""""""

In general, things like ``for`` loops should have no spaces between the ``for`` and opening brace (``(``). For example:

.. code:: c

   # incorrect
   for (int i = 0; i < 10; i++) { log_debug(i); }

   # correct
   for(int i = 0; i < 10; i++) { log_debug(i); }


Platform-Specific Tips
----------------------

Linux (Debian/Ubuntu)
"""""""""""""""""""""

On an apt-based system (Debian/Ubuntu), ensure you have build dependencies
installed:

.. code:: sh

    apt-get install meson cmake build-essential
    apt-get install libsdl2-dev libsdl2-mixer-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev

If your distribution of Linux uses Wayland as its default window server, ensure
that Wayland deps are installed:

.. code:: sh

    apt-get install libwayland-dev

For packaging, your best bet is ``.zip``. Invoke ``ninja`` to package a
``.zip``:

.. code:: sh

   ninja zip -C build/

macOS
"""""

On macOS, you need to begin with installing the Xcode Command Line Tools:

.. code:: sh

    xcode-select --install

There are additional command line tools that you'll need. You can acquire those
by using `Homebrew <https://brew.sh/>`__.

Follow the instructions for installing Homebrew, and then install the following
tools:

.. code:: sh

    brew install meson cmake pkg-config docutils imagemagick pygments

The following dependencies are technically optional, and can be pulled in at
build-time, but you're better off installing them yourself to reduce compile
times:

.. code:: sh

    brew install freetype2 libzip opusfile libvorbis webp sdl2

As of 2020-02-18, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly.
If you're having mysterious errors, ensure that they're not installed.

* ``spirv-tools``
* ``spirv-cross``
* ``sdl2_mixer``

Remove them with:

.. code:: sh

    brew remove spirv-tools spirv-cross sdl2_mixer

Taisei-compatible versions are bundled and will be pulled in at compile time.

In addition, if you're trying to compile on an older version of macOS
(i.e: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using the
following option:

.. code:: sh

   meson configure build/ --wrap-mode=forcefallback

Optionally, if you're on macOS and compiling for macOS, you can to install
`create-dmg <https://github.com/create-dmg/create-dmg>`__, which will allow
you to have nicer-looking macOS ``.dmg`` files for distribution:

.. code:: sh

    brew install create-dmg

You can create a ``.dmg`` on either Linux or macOS (although with ``create-dmg``
on macOS, the macOS-produced ``.dmg`` will look nicer):

.. code:: sh

   ninja dmg -C build/

Windows
"""""""

While the game itself officially supports Windows, building the project
directly on Windows is a bit difficult to set up due to the radically different
tooling required for a native Windows build environment.

However, you can still compile on a Windows-based computer by leveraging Windows
10's
`Windows For Linux (WSL) Subsystem <https://docs.microsoft.com/en-us/windows/wsl/install-win10>`__
to cross-compile to Windows. Ironically enough, compiling for Windows on Linux
ends up being easier and more consistent than trying to compile with Windows's
native toolset.

Taisei uses `mstorsjo/llvm-mingw <https://github.com/mstorsjo/llvm-mingw>`__ to
achieve cross-compiling on Windows. We also have a ``meson`` machine file
located at ``misc/ci/windows-llvm_mingw-x86_64-build-test-ci.ini`` to go with
that toolchain. In general, you'll need the following tools for compiling Taisei
for Windows on Linux:

-   ``llvm-mingw``
-   `nsis <https://nsis.sourceforge.io/Main_Page>`__ >= 3.0

On macOS, you're probably better off using Docker and the
`Docker container <https://hub.docker.com/r/mstorsjo/llvm-mingw/>`__ that
``llvm-mingw`` provides, and installing ``nsis`` on top of it. Refer to
``misc/ci/Dockerfile.windows`` for more insight.

Additionally, on Windows, you'll need to make sure you have *ANGLE support*
enabled, as previously mentioned.

Compiling Issues
----------------

-Wunused-variable
"""""""""""""""""

If you get an error compiling your code, but you're 100% sure that you've
actually used the variable, chances are you're using that variable in an
``assert()`` and are compiling with ``clang``.

``clang`` won't recognize that the variable is actually being used in an
``assert()``.

You can use the macro ``attr_unused`` to bypass that warning. This:

.. code:: c

    int x = 0;
    assert(x == 0);

Becomes this:

.. code:: c

    attr_unused int x = 0;
    assert(x == 0);
