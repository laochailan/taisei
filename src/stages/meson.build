
stages_src = []

stages = [
    'stage1',
    'stage2',
    'stage3',
    'stage4',
    'stage5',
    'stage6',
    'stagex',
]

foreach stage : stages
    subdir(stage)
    stages_src += get_variable('@0@_src'.format(stage))
endforeach

if is_developer_build
    stages_src += files(
        'dpstest.c',
        'corotest.c',
    )
endif
