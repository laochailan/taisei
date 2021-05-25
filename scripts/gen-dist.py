#!/usr/bin/env python3

from taiseilib.tempinstall import (
    temp_install,
)


from taiseilib.common import (
    add_common_args,
    run_main,
)

from taiseilib.releasefiles import gen_release_files

from pathlib import (
    Path,
    PurePosixPath,
)

from datetime import (
    datetime,
)

import argparse
import os
import shutil


def main(args):
    import logging
    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger(__name__)

    parser = argparse.ArgumentParser(description='Generate a distribution archive.', prog=args[0])

    parser.add_argument('build_dir',
        help='the build directory (defaults to CWD)',
        default=Path(os.getcwd()),
        type=Path,
        nargs='?',
    )

    parser.add_argument('output',
        help='the output file path',
        type=Path,
    )

    parser.add_argument('--format',
        help='the archive format',
        default='zip',
        choices=sorted(map(lambda x: x[0], shutil.get_archive_formats())),
    )

    parser.add_argument('--prefix',
        help='add a common prefix to all files and directories in the archive',
        default=None,
    )

    parser.add_argument('--release',
        help='Generate release files',
        dest='release',
        action='store_true',
    )

    add_common_args(parser)
    args = parser.parse_args(args[1:])

    if args.prefix is not None:
        p = PurePosixPath(args.prefix)

        if p.is_absolute() or p.is_reserved() or '..' in p.parts:
            raise ValueError('Bad prefix: {}'.format(args.prefix))

        args.prefix = str(p)

        if args.prefix == '.':
            args.prefix = None

    with temp_install(args.build_dir) as install_dir:
        if args.prefix is not None:
            os.chdir(str(install_dir.parent))
            install_dir.rename(install_dir.parent / args.prefix)
            archive = shutil.make_archive(str(args.output), args.format, '.', str(args.prefix))
        else:
            archive = shutil.make_archive(str(args.output), args.format, str(install_dir))

        archive = Path(archive)
        archive.rename(args.output)
        if args.release:
            gen_release_files(args.output)
            print("Generated release files (.sig, .sha256sum)")

    print("Generated distribution archive {}".format(str(args.output)))


if __name__ == '__main__':
    run_main(main)
