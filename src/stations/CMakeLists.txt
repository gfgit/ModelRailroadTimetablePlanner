add_subdirectory(manager)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  stations/stationsmatchmodel.h
  stations/stationssqlmodel.h

  stations/stationsmatchmodel.cpp
  stations/stationssqlmodel.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
