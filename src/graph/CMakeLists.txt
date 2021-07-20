add_subdirectory(model)
add_subdirectory(view)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  graph/graphmanager.h
  graph/graphmanager.cpp
  PARENT_SCOPE
)
