project(
    'janus-ftl-orchestrator',
    'cpp',
    default_options: [
        'cpp_std=c++2a',
        'cpp_args=-Wno-unknown-pragmas',
        'werror=true',
    ]
)

# Set the DEBUG define if we're a debug build
if get_option('buildtype').startswith('debug')
    add_project_arguments('-DDEBUG', language : 'cpp')
endif

sources = files([
    'src/Configuration.cpp',
    'src/main.cpp',
    'src/Orchestrator.cpp',
    'src/TlsConnectionManager.cpp',
])

# Pull in subprojects
fmt_wrap = subproject('fmt', default_options: 'default_library=static')
meson.override_dependency('fmt', fmt_wrap.get_variable('fmt_dep')) # Use our copy of fmt for spdlog
spdlog_wrap = subproject('spdlog', default_options: ['default_library=static', 'compile_library=true', 'external_fmt=true'] )
catch2_wrap = subproject('catch2')

deps = [
    dependency('libssl'),
    dependency('libcrypto'),
    # Meson wrapped dependencies
    fmt_wrap.get_variable('fmt_dep'),
    spdlog_wrap.get_variable('spdlog_dep'),
]

incdir = include_directories(['src', 'inc'])

executable(
    'janus-ftl-orchestrator',
    sources,
    cpp_pch: 'pch/pch.h',
    include_directories: incdir,
    dependencies: deps,
    install: true,
    install_dir: '/usr/local/bin',
)

testsources = files([
    # Test sources
    'test/test.cpp',
    # Unit tests
    'test/unit/FtlConnectionUnitTests.cpp',
    'test/unit/OrchestratorUnitTests.cpp',
    # Functional tests
    'test/functional/FunctionalTests.cpp',
    # Project sources
    'src/Orchestrator.cpp',
    'src/TlsConnectionManager.cpp',
])

testdeps = [
    dependency('libssl'),
    dependency('libcrypto'),
    catch2_wrap.get_variable('catch2_dep'),
    # Meson wrapped dependencies
    fmt_wrap.get_variable('fmt_dep'),
    spdlog_wrap.get_variable('spdlog_dep'),
]

executable(
    'janus-ftl-orchestrator-test',
    testsources,
    cpp_pch: 'pch/test_pch.h',
    include_directories: incdir,
    dependencies: testdeps,
)