#!/usr/bin/env python3

import re
from pathlib import Path
from collections import defaultdict

from taiseilib.common import (
    run_main,
)


mat_re = re.compile(r'(.*)_(diffuse|normal|ambient|roughness)\.(?:tex|basis|png|webp)$')


def main(args):
    resdir = Path(__file__).parent.parent / 'resources'

    materials = defaultdict(set)
    gfxdirs = []

    for pkgdir in resdir.glob('*.pkgdir'):
        gfx = pkgdir / 'gfx'

        if not gfx.is_dir():
            continue

        gfxdirs.append(gfx)
        print(gfx)

        for p in gfx.glob('**/*'):
            rel = p.relative_to(gfx)
            m = mat_re.match(str(rel))

            if m is None:
                continue

            matname, mapname = m.groups()
            materials[matname].add(mapname)

    gfxmain = gfxdirs[0]

    for mat, maps in materials.items():
        mat_filename = f'{mat}.material'

        skip = False
        for gfx in gfxdirs:
            if (gfx / mat_filename).exists():
                print(' - Skip', mat_filename)
                skip = True
                break
        if skip:
            continue

        outpath = (gfxmain / mat_filename).resolve()

        lines = ['']

        for mapname in sorted(maps):
            lines.append(f'{mapname}_map = {mat}_{mapname}')

        lines.append('')
        print(' * Write', outpath)
        outpath.write_text('\n'.join(lines))


if __name__ == '__main__':
    run_main(main)
