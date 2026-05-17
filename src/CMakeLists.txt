llvm_map_components_to_libnames(LLVM_LIBS core support native)

add_executable(cynide
      token.cpp
      lexer.cpp

      cynide.cpp
)

target_include_directories(cynide PRIVATE ./ ${LLVM_INCLUDE_DIRS})

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(cynide PRIVATE ${LLVM_DEFINITIONS_LIST})

target_link_libraries(cynide PRIVATE ${LLVM_LIBS})
