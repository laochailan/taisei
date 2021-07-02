#!/usr/bin/env python3

from taiseilib.tempinstall import (
    temp_install,
)

from taiseilib.common import (
    add_common_args,
    run_main,
    TaiseiError,
)

from taiseilib.configure import (
    add_configure_args,
    configure,
)

from taiseilib.releasefiles import gen_release_files

from pathlib import (
    Path,
    PureWindowsPath,
)

import argparse
import subprocess
import shlex
import os
import io


class MakeNSISError(TaiseiError, subprocess.SubprocessError):
    def __init__(self, returncode):
        super().__init__("makensis terminated with code {}".format(returncode))
        self.returncode = returncode


def main(args):
    parser = argparse.ArgumentParser(description='Generate an NSIS installer for Windows.', prog=args[0])

    parser.add_argument('build_dir',
        help='The build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    parser.add_argument('script_template',
        help='The NSIS script template',
        type=Path,
    )

    parser.add_argument('--release',
        help='Generate release files',
        dest='release',
        action='store_true',
    )

    add_configure_args(parser)
    add_common_args(parser)

    args = parser.parse_args(args[1:])
    args.variables = dict(args.variables)

    with args.script_template.open('r') as infile:
        template = infile.read()

    with temp_install(args.build_dir) as install_dir:
        glob  = tuple(install_dir.glob('**/*'))
        files = sorted((PureWindowsPath(p.relative_to(install_dir)) for p in glob if not p.is_dir()), reverse=True)
        dirs  = sorted((PureWindowsPath(p.relative_to(install_dir)) for p in glob if     p.is_dir()), reverse=True)

        indent = ' ' * 4
        instdir = '$INSTDIR'

        uninstall_files = ('\n'+indent).join('Delete "{}\\{}"'.format(instdir, path) for path in files)
        uninstall_dirs  = ('\n'+indent).join('RMDir "{}\\{}"'.format(instdir, path) for path in dirs)
        uninstall_commands = indent + ('\n\n'+indent).join((uninstall_files, uninstall_dirs))

        print(uninstall_commands)

        args.variables.update({
            'INSTALL_DIR': str(install_dir),
            'UNINSTALL_COMMANDS': uninstall_commands,
            'INSTALL_OPTIONS_INI': str(Path(args.rootdir) / 'scripts' / 'NSIS.InstallOptions.ini')
        })

        script = configure(template, args.variables,
            prefix='@',
            suffix='@',
            args=args
        )

        nsis_cmd = shlex.split('makensis -V3 -NOCD -INPUTCHARSET UTF8 -WX -')

        with subprocess.Popen(nsis_cmd, stdin=subprocess.PIPE, cwd=str(args.build_dir)) as proc:
            proc.stdin.write(script.encode('utf-8'))

        if proc.returncode != 0:
            raise MakeNSISError(proc.returncode)

        if args.release:
            gen_release_files(args.variables['OUTPUT'])
            print("Generated release files (.sig, .sha256sum)")

if __name__ == '__main__':
    run_main(main)
