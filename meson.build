project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'main.cpp',
    'JanusFtlOrchestrator.cpp',
    'OrchestrationConnection.cpp'
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

executable(
    'janus-ftl-orchestrator-test-client',
    files(['test-client.cpp']),
    dependencies: deps,
    cpp_args: cppargs
)