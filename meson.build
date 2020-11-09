project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'Configuration.cpp',
    'FtlConnection.cpp',
    'main.cpp',
    'Orchestrator.cpp',
    'Stream.cpp',
    'StreamStore.cpp',
    'TlsConnectionManager.cpp',
    'TlsConnectionTransport.cpp',
])

deps = [
    dependency('libssl'),
    dependency('libcrypto'),
    dependency('spdlog'),
]

cppargs = ['-std=c++20', '-Wno-unknown-pragmas', '-DCPPHTTPLIB_OPENSSL_SUPPORT']

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
    'test/unit/OrchestratorUnitTests.cpp',
    # Project sources
    'FtlConnection.cpp',
    'Orchestrator.cpp',
    'Stream.cpp',
    'StreamStore.cpp'
])

executable(
    'janus-ftl-orchestrator-test',
    testsources,
    dependencies: deps,
    cpp_args: cppargs
)