
resources_dir = meson.current_source_dir()

packages = [
    '00-taisei',
]

if host_machine.system() == 'emscripten'
    em_data_prefix = '/@0@'.format(config.get_unquoted('TAISEI_BUILDCONF_DATA_PATH'))
    em_bundles = ['gfx', 'misc']

    if get_option('a_default') != 'null'
        em_bundles += ['bgm', 'sfx']
    endif

    em_bundle_gfx_patterns = [
        'gfx/*.png',
        'gfx/*.webp',
        'gfx/*.basis',
        'gfx/*.basis.zst',
        'fonts/*.ttf',
        'fonts/*.otf',
    ]

    em_bundle_misc_patterns = [
        'gfx/*.ani',
        'gfx/*.spr',
        'gfx/*.tex',
        'gfx/*.material',
        'fonts/*.font',
        'models/*.iqm',

        # We don't want to include the shader sources here, we're going to translate them first.
        'shader/*.prog'
    ]

    em_bundle_bgm_patterns = [
        'bgm/*',
    ]

    em_bundle_sfx_patterns = [
        'sfx/*',
    ]

    foreach bundle : em_bundles
        set_variable('em_bundle_@0@_deps'.format(bundle), [])
        set_variable('em_bundle_@0@_files'.format(bundle), [])
        set_variable('em_bundle_@0@_packer_args'.format(bundle), [])
    endforeach

    # These are all text files that compress well
    em_bundle_misc_packer_args += ['--lz4']
endif

foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    pkg_zip = '@0@.zip'.format(pkg)
    pkg_path = join_paths(meson.current_source_dir(), pkg_pkgdir)

    subdir(pkg_pkgdir)

    if host_machine.system() == 'emscripten'
        foreach bundle : em_bundles
            var_patterns = 'em_bundle_@0@_patterns'.format(bundle)
            var_files = 'em_bundle_@0@_files'.format(bundle)
            var_packer_args = 'em_bundle_@0@_packer_args'.format(bundle)

            glob_result = run_command(glob_command, pkg_path, get_variable(var_patterns))
            assert(glob_result.returncode() == 0, 'Glob script failed')

            foreach file : glob_result.stdout().strip().split('\n')
                if file != ''
                    fpath = join_paths(meson.current_source_dir(), pkg_pkgdir, file)

                    set_variable(var_files, get_variable(var_files) + [fpath])

                    set_variable(var_packer_args, get_variable(var_packer_args) + [
                        '--preload', '@0@@@1@/@2@'.format(fpath, em_data_prefix, file)
                    ])
                endif
            endforeach
        endforeach
    elif package_data
        bindist_deps += custom_target(pkg_zip,
            command : [pack_command,
                pkg_path,
                '@OUTPUT@',
                '--depfile', '@DEPFILE@',
                '--exclude', '**/meson.build',
            ],
            output : pkg_zip,
            depfile : '@0@.d'.format(pkg_zip),
            install : true,
            install_dir : data_path,
            console : true,
        )
    else
        glob_result = run_command(glob_command, pkg_path, '**/meson.build')
        assert(glob_result.returncode() == 0, 'Glob script failed')
        install_subdir(pkg_pkgdir, install_dir : data_path, exclude_files : glob_result.stdout().split('\n'))
    endif
endforeach

if host_machine.system() == 'nx'
    # Package shaders that were transpiled
    shader_pkg_zip = '01-es-shaders.zip'
    shader_pkg_path = join_paths(shaders_build_dir, '..')
    if package_data
        bindist_deps += custom_target(shader_pkg_zip,
            command : [pack_command,
                shader_pkg_path,
                '@OUTPUT@',
                '--depfile', '@DEPFILE@',
                '--exclude', '**/*.spv',
                '--exclude', '**/meson.build',
            ],
            output : shader_pkg_zip,
            depends : essl_targets,
            depfile : '@0@.d'.format(shader_pkg_zip),
            install : true,
            install_dir : data_path,
            console : true,
        )
    else
        glob_result = run_command(glob_command, shaders_build_dir, '**/*.spv', '**/meson.build')
        assert(glob_result.returncode() == 0, 'Glob script failed')
        install_subdir(shaders_build_dir, install_dir : data_path, exclude_files : glob_result.stdout().split('\n'))
    endif
endif

if host_machine.system() == 'emscripten'
    # First add some stuff that isn't sourced from resources/

    foreach shader : essl_targets
        # This one is especially dirty!
        abs_path = shader.full_path()
        assert(abs_path.startswith(shaders_build_dir), 'Assumption about shader output location violated')
        rel_path = '@0@/shader@1@'.format(em_data_prefix, abs_path.split(shaders_build_dir)[1])

        em_bundle_misc_deps += shader
        em_bundle_misc_packer_args += [
            '--preload', '@0@@@1@'.format(abs_path, rel_path)
        ]
    endforeach

    packer = find_program('file_packager')  # should be provided by cross file
    em_bundle_link_args = []  # We'll pass these to the final emcc linking step

    # Finally, set up build targets for our bundles

    foreach bundle : em_bundles
        var_files = 'em_bundle_@0@_files'.format(bundle)
        var_deps = 'em_bundle_@0@_deps'.format(bundle)
        var_packer_args = 'em_bundle_@0@_packer_args'.format(bundle)

        data_name = 'bundle_@0@.data'.format(bundle)
        loader_name = 'bundle_@0@.js'.format(bundle)
        out_loader = join_paths(meson.current_build_dir(), '@0@.raw'.format(loader_name))

        bundle_data = custom_target(data_name,
            command : [
                packer,
                '@OUTPUT0@',
                '--js-output=@0@'.format(out_loader),  # No, this one does not accept "--js-output foobar.js"
                '--use-preload-cache',
                '--from-emcc',
                '--no-node',
                get_variable(var_packer_args)
            ],
            output : data_name,
            depends : get_variable(var_deps),
            depend_files : get_variable(var_files),
            install : true,
            install_dir : bindir,
        )

        bundle_loader = custom_target(loader_name,
            command : [
                em_set_bundle_uuid_command,
                out_loader,
                '--sha1', bundle_data,
                '--output', '@OUTPUT@',
            ],
            output : loader_name,
            install : false,
        )

        em_bundle_link_args += ['--pre-js', bundle_loader]
    endforeach
endif
