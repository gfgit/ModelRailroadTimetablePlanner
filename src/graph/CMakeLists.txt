add_subdirectory(model)
add_subdirectory(view)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  graph/graphmanager.h
  graph/graphmanager.cpp

  graph/linegraphtypes.h
  PARENT_SCOPE
)
