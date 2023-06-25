add_subdirectory(model) add_subdirectory(view)

  set(MR_TIMETABLE_PLANNER_SOURCES ${MR_TIMETABLE_PLANNER_SOURCES} graph / linegraphtypes.h graph
      / linegraphtypes.cpp PARENT_SCOPE)
