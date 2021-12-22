add_subdirectory(model)
add_subdirectory(pages)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}

  stations/importer/stationimportwizard.h
  stations/importer/stationimportwizard.cpp

  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
