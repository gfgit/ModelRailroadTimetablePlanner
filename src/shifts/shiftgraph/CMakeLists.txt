add_subdirectory(model)
add_subdirectory(view)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}

  shifts/shiftgraph/jobchangeshiftdlg.h
  shifts/shiftgraph/jobchangeshiftdlg.cpp

  shifts/shiftgraph/shiftgrapheditor.h
  shifts/shiftgraph/shiftgrapheditor.cpp

  PARENT_SCOPE
)
