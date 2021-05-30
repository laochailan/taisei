#!/usr/bin/env python3

from taiseilib.tempinstall import temp_install
from taiseilib.releasefiles import gen_release_files
from taiseilib.common import run_main

from pathlib import Path

import argparse
import subprocess
import shlex
import os
from sys import platform


def main(args):
    parser = argparse.ArgumentParser(description='Generate a .dmg package for macOS.', prog=args[0])

    parser.add_argument('output',
        help='The destination .dmg file',
    )

    parser.add_argument('build_dir',
        help='The build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    parser.add_argument('--release',
        help='Generate release files',
        dest='release',
        action='store_true',
    )

    args = parser.parse_args(args[1:])

    with temp_install(args.build_dir) as install_path:
        # for compiling on Darwin as 'genisoimage' doesn't exist
        # uses 'create-dmg' (brew install create-dmg)
        if platform == "darwin":
            command = shlex.split('''create-dmg
                --volname "Taisei"
                --volicon "Taisei.app/Contents/Resources/Taisei.icns"
                --window-pos 200 120
                --window-size 550 480
                --icon-size 64
                --icon "Taisei.app" 100 50
                --icon "README.txt" 50 200
                --icon "STORY.txt" 200 200
                --icon "GAME.html" 350 200
                --icon "COPYING" 125 350
                --icon "ENVIRON.html" 275 350
                --hide-extension "Taisei.app"
                --app-drop-link 300 50''') + [args.output, str(install_path)]
        else:
            (install_path / 'Applications').symlink_to('/Applications')

            command = shlex.split('genisoimage -V Taisei -D -R -apple -no-pad -o') + [
                args.output,
                str(install_path),
            ]

        subprocess.check_call(command, cwd=str(install_path))
        if args.release:
            gen_release_files(args.output)
            print('\nGenerated release files (.sig, .sha256sum)')

        print('\nPackage generated:', args.output)


if __name__ == '__main__':
    run_main(main)
