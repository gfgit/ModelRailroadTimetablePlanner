add_subdirectory(helper)
add_subdirectory(wizard)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  printing/printdefs.h

  printing/printworker.h
  printing/printworker.cpp

  printing/printworkerhandler.h
  printing/printworkerhandler.cpp

  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
