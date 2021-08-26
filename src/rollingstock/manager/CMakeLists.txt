add_subdirectory(delegates)
add_subdirectory(dialogs)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  rollingstock/rollingstockmatchmodel.h
  rollingstock/manager/rollingstockmanager.h

  rollingstock/manager/rollingstockmanager.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
