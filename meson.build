project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'Configuration.cpp',
    'main.cpp',
    'Orchestrator.cpp',
    'TlsConnection.cpp',
    'TlsConnectionManager.cpp'
])

deps = [
    dependency('libssl'),
    dependency('libcrypto'),
    dependency('spdlog')
]

cppargs = ['-Wno-unknown-pragmas', '-DCPPHTTPLIB_OPENSSL_SUPPORT']

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
    'test/OrchestratorTests.cpp',
    # Project sources
    'Orchestrator.cpp'
])

executable(
    'janus-ftl-orchestrator-test',
    testsources,
    dependencies: deps,
    cpp_args: cppargs
)