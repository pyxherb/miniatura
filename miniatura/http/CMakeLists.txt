file(GLOB HEADER *.h)
file(GLOB SRC *.cc)

target_sources(miniatura PRIVATE ${HEADER} ${SRC})
