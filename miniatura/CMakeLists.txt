find_package(Boost REQUIRED)

file(GLOB HEADER *.h)
file(GLOB SRC *.cc)

add_library(miniatura STATIC)
target_sources(miniatura PRIVATE ${HEADER} ${SRC})
set_target_properties(miniatura PROPERTIES CXX_STANDARD 17)

target_include_directories(miniatura PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(miniatura PUBLIC ${BOOST_LIBRARIES})

add_subdirectory("http")
