add_contract(tokenx.mm tokenx.mm ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenx.mm.cpp)

target_include_directories(tokenx.mm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(tokenx.mm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/tokenx.mm.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/tokenx.mm.contracts.md @ONLY )

target_compile_options( tokenx.mm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
