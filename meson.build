project(
    'getbar',
    'c',
    version: run_command(
        'sh',
        '-c', '''
        v=$(git describe --tags --always --dirty 2>/dev/null || true)
        v=${v#v}
        if [ -n "$v" ]; then
            if [ "${v%.*}" = "$v" ]; then
                v="0.0.0-$v"
            fi
            printf "%s" "$v"
        else
            echo "1.0.0"
        fi
        ''',
        check: false,
    ).stdout().strip(),
    license: 'AGPL-3.0-or-later',
    default_options: [
        'c_std=c11',
        'warning_level=2',
    ],
)

project_author = 'Lenik'
project_email = 'getbar@bodz.net'
project_year = 2026

fs = import('fs')
c = meson.get_compiler('c')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
mandir = prefix / get_option('mandir')
pkgdatadir = datadir / meson.project_name()
pkgdocdir = datadir / 'doc' / meson.project_name()

config_h = configuration_data()
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_AUTHOR', project_author)
config_h.set_quoted('PROJECT_EMAIL', project_email)
config_h.set('PROJECT_YEAR', project_year)
config_h.set_quoted('LOCALEDIR', localedir)

configure_file(
    output: 'config.h',
    configuration: config_h,
)

bas_c_static_dep = dependency('bas-c-static', required: true)
curl_dep = dependency('libcurl', required: true)

inc = include_directories('src')
c_args = [
    '-std=c11',
    '-DTEXT_DOMAIN=getbar',
]

subdir('po')

executable(
    'getbar',
    'src/getbar.c',
    c_args: c_args,
    include_directories: inc,
    dependencies: [curl_dep, bas_c_static_dep],
    install: true,
    install_dir: bindir,
)

install_data(
    'gnuplot.rc.example',
    install_dir: pkgdatadir,
)

install_data(
    'getbar.bash',
    install_dir: datadir / 'bash-completion' / 'completions',
    rename: 'getbar',
)

readme_langs = ['zh_CN', 'zh_TW', 'ja', 'ko', 'th', 'vi', 'fr', 'de', 'it', 'eo']

doc_install = ['LICENSE', 'README.md']
foreach lang : readme_langs
    doc_install += 'README-' + lang + '.md'
endforeach

install_data(
    doc_install,
    install_dir: pkgdocdir,
)

install_data(
    'images/chart.png',
    install_dir: pkgdocdir / 'images',
)

pandoc = find_program('pandoc', required: false)
lualatex = find_program('lualatex', required: false)
doc_resource_path = meson.current_source_dir()
chart_png = files('images/chart.png')

pdf_target_names = []

if pandoc.found() and lualatex.found()
    custom_target(
        'README-pdf',
        input: 'README.md',
        output: 'README.pdf',
        command: [
            'md2luapdf',
            '--srcdir', doc_resource_path,
            '@INPUT@',
            '@OUTPUT@',
        ],
        depend_files: chart_png,
        build_by_default: true,
        install: true,
        install_dir: pkgdocdir,
    )
    pdf_target_names += 'README.pdf'
elif not pandoc.found()
    message('pandoc not found; skipping README PDF generation')
else
    message('lualatex not found; skipping README PDF generation')
endif

foreach lang : readme_langs
    readme_md = 'README-' + lang + '.md'
    readme_pdf_name = 'README-' + lang + '.pdf'
    if not fs.is_file(meson.current_source_dir() / readme_md)
        message('missing @0@; skipping PDF generation'.format(readme_md))
        continue
    endif
    if not pandoc.found() or not lualatex.found()
        continue
    endif
    custom_target(
        'README-' + lang + '-pdf',
        input: readme_md,
        output: readme_pdf_name,
        command: [
            'md2luapdf',
            '--srcdir', doc_resource_path,
            '--lang', lang,
            '@INPUT@',
            '@OUTPUT@',
        ],
        depend_files: chart_png,
        build_by_default: true,
        install: true,
        install_dir: pkgdocdir,
    )
    pdf_target_names += readme_pdf_name
endforeach

if pdf_target_names.length() > 0
    run_target(
        'readme-pdfs',
        command: [
            'ninja', '-C', '@BUILD_ROOT@',
        ] + pdf_target_names,
    )
endif

config_man = configuration_data()
config_man.set('PROJECT_VERSION', meson.project_version())
config_man.set('PROJECT_AUTHOR', project_author)
config_man.set('PROJECT_EMAIL', project_email)
config_man.set('PROJECT_YEAR', project_year)

configure_file(
    input: 'getbar.1.in',
    output: 'getbar.1',
    configuration: config_man,
    install: true,
    install_dir: mandir / 'man1',
)

manpage_langs = ['de', 'eo', 'fr', 'it', 'ja', 'ko', 'th', 'vi', 'zh_CN', 'zh_TW']
foreach lang : manpage_langs
    manin = 'man' / lang / 'getbar.1.in'
    if fs.is_file(meson.current_source_dir() / manin)
        man_gen = configure_file(
            input: manin,
            output: 'getbar.1.' + lang,
            configuration: config_man,
        )
        install_data(
            man_gen,
            install_dir: mandir / lang / 'man1',
            rename: 'getbar.1',
        )
    endif
endforeach

run_target(
    'look',
    command: [
        'bash',
        '-euc', '''
            tmpdir=$(mktemp -d)
            DESTDIR=$tmpdir meson install -C "@BUILD_ROOT@"
            tree "$tmpdir"
            rm -fr "$tmpdir"
        ''',
    ],
)

run_target(
    'install-symlinks',
    command: [
        'bash',
        '-euc', '''
            prefix="@0@"
            bindir="@1@"
            datadir="@2@"
            mandir="@3@"

            mkdir -p "$bindir" "$datadir/bash-completion/completions" "$mandir/man1"

            sudo ln -sfn "@BUILD_ROOT@/getbar" "$bindir/getbar"
            sudo ln -sfn "@BUILD_ROOT@/getbar.1" "$mandir/man1/getbar.1"
            sudo ln -sfn "@SOURCE_ROOT@/getbar.bash" "$datadir/bash-completion/completions/getbar"

            printf "Symlinks installed under %s\n" "$prefix"
        '''.format(
            get_option('prefix'),
            bindir,
            datadir,
            mandir,
        ),
    ],
)

run_target(
    'uninstall-symlinks',
    command: [
        'bash',
        '-euc', '''
            bindir="@0@"
            datadir="@1@"
            mandir="@2@"

            for p in \
                "$bindir/getbar" \
                "$mandir/man1/getbar.1" \
                "$datadir/bash-completion/completions/getbar"
            do
                if [ -L "$p" ]; then
                    sudo rm -f "$p"
                    printf "Removed %s\n" "$p"
                else
                    printf "Skipped (not a symlink): %s\n" "$p"
                fi
            done
        '''.format(
            bindir,
            datadir,
            mandir,
        ),
    ],
)
