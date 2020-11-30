project('janus-ftl-orchestrator', 'cpp')

sources = files([
    'src/Configuration.cpp',
    'src/FtlOrchestratorService.cpp',
    'src/Signal.cpp',
    'src/NodeStore.cpp',
])

cmake = import('cmake')

opt_var = cmake.subproject_options()

opt_var.add_cmake_defines({
    'gRPC_INSTALL': 'OFF',
    'gRPC_BUILD_TESTS': 'OFF',
    'gRPC_AS_SUBMODULE': 'ON',
    'gRPC_SSL_PROVIDER': 'package',
    'CMAKE_BUILD_TYPE': 'Release'
})

opt_var.set_install(true, target: 'grpc_cpp_plugin')

grpc_proj = cmake.subproject('grpc', options: opt_var)

# Fetch code generator executables
protoc = grpc_proj.target('protoc')
grpc_cpp_plugin = grpc_proj.target('grpc_cpp_plugin')

message(protoc.full_path())
message(grpc_cpp_plugin.full_path())

gen = generator(protoc,
    output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h', '@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
    arguments : [
        '--proto_path=@CURRENT_SOURCE_DIR@/protobuf',
        '--cpp_out=@BUILD_DIR@',
        '--grpc_out=@BUILD_DIR@',
        '--plugin=protoc-gen-grpc=@0@'.format(grpc_cpp_plugin.full_path()),
        '@INPUT@'])

sources += gen.process('protobuf/ftl_orchestrator.proto')

grpc_deps = [
    dependency('libcrypto'),
    dependency('libssl'),
    dependency('threads'),
    grpc_proj.dependency('absl_bad_optional_access'),
    grpc_proj.dependency('absl_bad_variant_access'),
    grpc_proj.dependency('absl_base'),
    grpc_proj.dependency('absl_city'),
    grpc_proj.dependency('absl_civil_time'),
    grpc_proj.dependency('absl_cord'),
    grpc_proj.dependency('absl_debugging_internal'),
    grpc_proj.dependency('absl_demangle_internal'),
    grpc_proj.dependency('absl_dynamic_annotations'),
    grpc_proj.dependency('absl_exponential_biased'),
    grpc_proj.dependency('absl_graphcycles_internal'),
    grpc_proj.dependency('absl_hash'),
    grpc_proj.dependency('absl_hashtablez_sampler'),
    grpc_proj.dependency('absl_int128'),
    grpc_proj.dependency('absl_log_severity'),
    grpc_proj.dependency('absl_malloc_internal'),
    grpc_proj.dependency('absl_raw_hash_set'),
    grpc_proj.dependency('absl_raw_logging_internal'),
    grpc_proj.dependency('absl_spinlock_wait'),
    grpc_proj.dependency('absl_stacktrace'),
    grpc_proj.dependency('absl_status'),
    grpc_proj.dependency('absl_str_format_internal'),
    grpc_proj.dependency('absl_strings_internal'),
    grpc_proj.dependency('absl_strings'),
    grpc_proj.dependency('absl_symbolize'),
    grpc_proj.dependency('absl_synchronization'),
    grpc_proj.dependency('absl_throw_delegate'),
    grpc_proj.dependency('absl_time_zone'),
    grpc_proj.dependency('absl_time'),
    grpc_proj.dependency('address_sorting'),
    grpc_proj.dependency('c-ares'),
    grpc_proj.dependency('gpr'),
    grpc_proj.dependency('grpc'),
    grpc_proj.dependency('grpc++'),
    grpc_proj.dependency('libprotobuf'),
    grpc_proj.dependency('re2'),
    grpc_proj.dependency('upb'),
    grpc_proj.dependency('zlibstatic'),
]

deps = [
    dependency('catch2', fallback: ['catch2', 'catch2_dep']),
    subproject('spdlog').get_variable('spdlog_dep'), # use wrapped copy of spdlog
]

cppargs = ['-std=c++20', '-Wno-unknown-pragmas']
incdir = include_directories(['src', 'inc'])

executable(
    'janus-ftl-orchestrator',
    sources,
    include_directories: incdir,
    dependencies: deps + grpc_deps,
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
    # 'src/TlsConnectionManager.cpp',
])

# executable(
#     'janus-ftl-orchestrator-test',
#     testsources,
#     include_directories: incdir,
#     dependencies: deps,
#     cpp_args: cppargs
# )

# executable(
#     'janus-ftl-orchestrator-test-client',
#     files(['src/test-client.cpp']),
#     dependencies: deps,
#     cpp_args: cppargs
# )
