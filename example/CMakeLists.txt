file(GLOB HEADER *.h)
file(GLOB SRC *.cc)

add_executable(httpsrv)
target_sources(httpsrv PRIVATE ${HEADER} ${SRC})
set_target_properties(httpsrv PROPERTIES CXX_STANDARD 17)
target_link_libraries(httpsrv PRIVATE miniatura)
