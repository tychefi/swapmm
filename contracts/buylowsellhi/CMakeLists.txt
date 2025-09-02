add_contract(buylowsellhi buylowsellhi ${CMAKE_CURRENT_SOURCE_DIR}/src/buylowsellhi.cpp)

target_include_directories(buylowsellhi
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(buylowsellhi
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/buylowsellhi.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/buylowsellhi.contracts.md @ONLY )

target_compile_options( buylowsellhi PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
target_link_libraries(buylowsellhi flon_base)