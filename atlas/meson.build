
atlases_dir = meson.current_source_dir()
atlases_overrides_dir = join_paths(atlases_dir, 'overrides')
resources_gfx_dir = join_paths(resources_pkg_main, 'gfx')

# Args are applied in this order:
#   1. Common args
#   2. Atlas-specific
#   3. Profile-specific

preset_webp = [
    # Leanify doesn't handle webp (yet?), but it'll work on alphamaps (they are always png)
    '--leanify',
    '--format', 'webp',
]

preset_png = [
    '--leanify',
    '--format', 'png',
]

preset_fast = [
    # webp encoding is slower (but more efficient)
    '--no-leanify',
    '--format', 'png',
]

atlas_common_args = [
    '--width=4096',
    '--height=4096',
    '--multiple',
    '--crop',

    preset_webp,

    # Padding to prevent filter bleeding.
    # 1 is normally enough, but not when mipmaps are involved.
    '--border=2'
]

atlases = [
    ['common',      []],
    ['common_ui',   ['--width=1024', '--height=1024']],
    ['huge',        []],
    ['portraits',   ['--width=4096', '--height=4096']],
    ['temp_stagex', []],
]

atlas_profiles = [
    ['',        []],
    ['-fast',   [preset_fast]],
    ['-png',    [preset_png]],
    ['-webp',   [preset_webp]],
]

foreach profile : atlas_profiles
    profile_suffix = profile[0]
    profile_args = profile[1]
    profile_targets = []

    foreach atlas : atlases
        atlas_name = atlas[0]
        atlas_args = atlas[1]
        atlas_target = 'gen-atlas-@0@@1@'.format(atlas_name, profile_suffix)
        profile_targets += atlas_target

        run_target(atlas_target,
            command : [
                gen_atlas_command,
                atlases_overrides_dir,
                join_paths(atlases_dir, atlas_name),
                resources_gfx_dir,
                atlas_common_args,
                atlas_args,
                profile_args,
            ],
        )
    endforeach

    run_target('gen-atlases@0@'.format(profile_suffix),
        command : [
            gen_atlases_command,
            profile_targets,
        ],
    )
endforeach
