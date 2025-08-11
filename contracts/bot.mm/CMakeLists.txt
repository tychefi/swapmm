add_contract(bot.mm bot.mm ${CMAKE_CURRENT_SOURCE_DIR}/src/bot.mm.cpp)

target_include_directories(bot.mm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(bot.mm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/bot.mm.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/bot.mm.contracts.md @ONLY )

target_compile_options( bot.mm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
