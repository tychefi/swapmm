add_contract(tokenx.mm tokenx.mm ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenx.mm.cpp)

target_include_directories(tokenx.mm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(tokenx.mm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( tokenx.mm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
target_link_libraries(tokenx.mm flon_base)