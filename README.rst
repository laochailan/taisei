Taisei Project
==============

.. image:: misc/icons/taisei.ico
   :width: 150
   :alt: Taisei Project icon

.. contents::

Introduction
------------

About Taisei Project
^^^^^^^^^^^^^^^^^^^^

Taisei Project is an open source fan-game set in the world of
`Tōhō Project <https://en.wikipedia.org/wiki/Touhou_Project>`__.
It is a top-down vertical-scrolling curtain fire shooting game (STG), also known
as a "bullet hell" or "danmaku. It is a fast-paced game focused around pattern
recognition and mastery through practice.

Taisei Project is highly portable, and is written in C11, using SDL2 with an
OpenGL renderer. It is officially supported on Windows, Linux, macOS, and
through WebGL-enabled browsers such as Firefox and Chromium-based browsers
(Chrome, Edge, etc). It can also be compiled for a number of other operating
systems.

For gameplay screenshots, see
`our website <https://taisei-project.org/media>`__.

For gameplay instructions, read `this <doc/GAME.rst>`__.

For the story, read `this <doc/STORY.txt>`__. (Spoiler warning!)

About Tōhō Project
^^^^^^^^^^^^^^^^^^

Tōhō Project is an indie game series (also known as "doujin" in Japanese)
known for its ensemble cast of characters and memorable soundtracks.
It is produced by and large by a single artist known as ZUN, and has a
`permissive license <https://en.touhouwiki.net/wiki/Touhou_Wiki:Copyrights#Copyright_status.2FTerms_of_Use_of_the_Touhou_Project>`__
which allows for indie derivative works such as Taisei Project to legally exist.

Taisei is *not* a "clone" of Tōhō, and tells an original story with its own
music, art, gameplay mechanics, and codebase. While some familiarity with Tōhō
is helpful, the gameplay can be enjoyed on its own without prior knowledge of
the series.

For more information on dōjin culture,
`click here <https://en.wikipedia.org/wiki/D%C5%8Djin>`__.

Installation
------------

You can find precompiled binaries of the complete game on the
`Releases <https://github.com/taisei-project/taisei/releases>`__ page on
GitHub, available for Windows (x86/x64), Linux, and macOS.

An experimental build for Nintendo Switch (homebrew) also exists (use at your
own risk).

You can also play our experimental WebGL build through your web browser
`here <https://play.taisei-project.org/>`__. (Chromium-based browsers and
Firefox supported.)

Source Code & Development
-------------------------

Basic Dependencies
^^^^^^^^^^^^^^^^^^

Run-Time Dependencies
_____________________

Required
********

-  OpenGL >= 3.3, or OpenGL ES >= 3.0
-  SDL2 >= 2.0.10
-  cglm >= 0.7.8
-  libpng >= 1.5.0
-  libwebpdecoder >= 0.5 or libwebp >= 0.5
-  libzip >= 1.5.0 (>= 1.7.0 recommended)
-  libzstd >= 1.4.0
-  freetype2
-  opusfile
-  zlib

Optional
********

-  OpenSSL (for a better SHA-256 implementation; used in shader cache)
-  SPIRV-Cross >= 2019-03-22 (for OpenGL ES backends)
-  libshaderc (for OpenGL ES backends)
-  `ANGLE <https://github.com/google/angle>`__ (useful for platforms with
   flaky/non-existent OpenGL support, such as Windows)
-  GameMode headers (Linux only; for automatic
   `GameMode <https://github.com/FeralInteractive/gamemode>`__ integration)

Build-Time Dependenices
_______________________

-  ``gcc`` or ``clang``
-  meson >= 0.53.0
-  Python >= 3.6
-  `python-zstandard <https://github.com/indygreg/python-zstandard>`__ >= 0.11.1
-  `python-docutils <https://pypi.org/project/docutils/>`__ (optional, for generating documentation)

Obtaining Source Code
^^^^^^^^^^^^^^^^^^^^^

Source
______

We recommend fetching the source code using ``git``:

::

    git clone --recurse-submodules https://github.com/taisei-project/taisei

You should also run ``git submodule update`` whenever you pull in
new code, checkout another branch, or perform any ``git`` actions. The ``./pull``
and ``./checkout`` helper scripts can do that for you automatically.

Archive
_______

⚠️ **NOTE**: Due to the way GitHub packages source code, the ``Download ZIP``
link on the main repo *does not work*.

This is due to the fact that GitHub does not package submodules alongside source
code when it automatically generates ``.zip`` files. We've instead created those
archives manually, and you **MUST** download the archive from
`Releases <https://github.com/taisei-project/taisei/releases>`__ page.

Compiling Source Code
^^^^^^^^^^^^^^^^^^^^^

Currently, we recommend building Taisei on a POSIX-like system (Linux, macOS,
etc).

While Taisei is highly configurable, the easiest way to compile the code for
your host machine is:

::

    meson setup build/
    meson compile -C build/
    meson install -C build/

Replays, Screenshots, and Settings Locations
--------------------------------------------

Taisei stores all data in a platform-specific directory:

-  On **Windows**, this will probably be ``%APPDATA%\taisei``
-  On **macOS**, it's ``$HOME/Library/Application Support/taisei``
-  On **Linux**, **\*BSD**, and most other **Unix**-like systems, it's
   ``$XDG_DATA_HOME/taisei`` or ``$HOME/.local/share/taisei``

This is referred to as the **Storage Directory**. You can set the environment
variable ``TAISEI_STORAGE_PATH`` to override this behaviour.

Troubleshooting
---------------

Feel free to
`open up an issue <https://github.com/taisei-project/taisei/issues>`__ if you
run into any issues with compiling or running Taisei.

Contact
-------

-  https://taisei-project.org/

-  `Our server on Discord <https://discord.gg/JEHCMzW>`__
