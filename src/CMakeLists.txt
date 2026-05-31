llvm_map_components_to_libnames(LLVM_LIBS core support native)

add_executable(cylang
      token.cpp
      lexer.cpp
      parser.cpp
      sema.cpp
      cylang.cpp
)

target_include_directories(cylang PRIVATE ./ ${LLVM_INCLUDE_DIRS})

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(cylang PRIVATE ${LLVM_DEFINITIONS_LIST})

target_link_libraries(cylang PRIVATE ${LLVM_LIBS})
