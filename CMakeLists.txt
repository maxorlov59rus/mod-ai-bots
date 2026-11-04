#
# mod-ai-bots — модуль AI-ботов для AzerothCore
#

CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)

add_library(mod-ai-bots STATIC ${PRIVATE_SOURCES})

CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} PUBLIC_INCLUDES)

target_include_directories(mod-ai-bots
  PUBLIC
    ${PUBLIC_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mod-ai-bots
  PRIVATE
    game)

add_dependencies(mod-ai-bots revision.h)

if (USE_COREPCH)
  add_cxx_pch(mod-ai-bots)
endif()
