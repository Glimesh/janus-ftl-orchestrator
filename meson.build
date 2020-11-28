project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'src/Configuration.cpp',
    'src/main.cpp',
    'src/Orchestrator.cpp',
    'src/TlsConnectionManager.cpp',
])

deps = [
    dependency('libssl'),
    dependency('libcrypto'),
    dependency('spdlog'),
    dependency('catch2', fallback: ['catch2', 'catch2_dep']),
]

cppargs = ['-std=c++20', '-Wno-unknown-pragmas']
incdir = include_directories(['src', 'inc'])

executable(
    'janus-ftl-orchestrator',
    sources,
    include_directories: incdir,
    dependencies: deps,
    cpp_args: cppargs,
    install: true,
    install_dir: '/usr/local/bin'
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

executable(
    'janus-ftl-orchestrator-test',
    testsources,
    include_directories: incdir,
    dependencies: deps,
    cpp_args: cppargs
)