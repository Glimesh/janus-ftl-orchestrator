project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'src/Configuration.cpp',
    'src/FtlConnection.cpp',
    'src/main.cpp',
    'src/Orchestrator.cpp',
    'src/TlsConnectionManager.cpp',
    'src/TlsConnectionTransport.cpp',
])

deps = [
    dependency('libssl'),
    dependency('libcrypto'),
    dependency('spdlog'),
    dependency('catch2', fallback: ['catch2', 'catch2_dep']),
]

cppargs = ['-std=c++20', '-Wno-unknown-pragmas']

executable(
    'janus-ftl-orchestrator',
    sources,
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
    # Project sources
    'src/FtlConnection.cpp',
    'src/Orchestrator.cpp',
])

executable(
    'janus-ftl-orchestrator-test',
    testsources,
    dependencies: deps,
    cpp_args: cppargs
)