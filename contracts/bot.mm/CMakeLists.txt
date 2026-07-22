add_contract(bot.mm bot.mm ${CMAKE_CURRENT_SOURCE_DIR}/src/bot.mm.cpp)

target_include_directories(bot.mm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(bot.mm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(bot.mm flon_base)

target_ricardian_directory(bot.mm ${CMAKE_CURRENT_SOURCE_DIR}/ricardian)
