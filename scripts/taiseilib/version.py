
from . import common

import sys
import subprocess
import shlex
import re


class VerionFormatError(common.TaiseiError):
    pass


class Version(object):
    regex = re.compile(r'^v?(\d+)(?:\.(\d+)(?:\.(\d+))?)?(?:[-+](\d+))?(?:[-+](.*))?$')

    def _make_string(self):
        s = str(self.major) + "." + str(self.minor)

        if self.patch > 0:
            s += "." + str(self.patch)

        if self.tweak > 0:
            s += "-" + str(self.tweak)

        if self.note is not None:
            s += "-" + self.note

        return s

    def __init__(self, version_str):
        match = self.regex.match(version_str)

        if match is None:
            raise VerionFormatError("Error: Malformed version string '{0}'. Please use the following format: [v]major[.minor[.patch]][-tweak][-extrainfo]".format(version_str))

        def mkint(val):
            if val is None:
                return 0
            return int(val)

        ma, mi, pa, tw, no = match.groups()

        self.major = mkint(ma)
        self.minor = mkint(mi)
        self.patch = mkint(pa)
        self.tweak = mkint(tw)
        self.note = no
        self.string = self._make_string()
        self.full_string = "Taisei v{0}".format(self.string)

    def format(self, template='{string}'):
        return template.format(**self.__dict__)


def get(*, rootdir=None, fallback=None, args=common.default_args):
    rootdir = rootdir if rootdir is not None else args.rootdir
    fallback = fallback if fallback is not None else args.fallback_version

    if rootdir is None:
        import pathlib
        rootdir = pathlib.Path(__file__).parent

    try:
        version_str = subprocess.check_output(
            shlex.split('git describe --dirty --match v[0-9]*[!asz]'),
            cwd=str(rootdir),
            universal_newlines=True
        ).strip()

        if '-' in version_str:
            version_str += '-' + subprocess.check_output(
                shlex.split('git rev-parse --abbrev-ref HEAD'),
                cwd=str(rootdir),
                universal_newlines=True
            ).strip()
    except (subprocess.SubprocessError, OSError) as e:
        if not fallback:
            raise

        print(e, file=sys.stderr)
        print("Warning: git not found or not a git repository; using fallback version {0}".format(fallback), file=sys.stderr)
        version_str = fallback

    return Version(version_str)


def main(args):
    import argparse

    parser = argparse.ArgumentParser(description='Print the Taisei version.', prog=args[0])

    parser.add_argument('format', type=str, nargs='?', default='{string}',
        help='format string; variables: major, minor, patch, tweak, note, string, full_string')

    common.add_common_args(parser)

    args = parser.parse_args(args[1:])
    print(get(args=args).format(template=args.format))
