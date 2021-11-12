add_subdirectory(free_rs_viewer)
add_subdirectory(lines)
add_subdirectory(segments)
add_subdirectory(stations)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  stations/manager/stationplanmodel.h
  stations/manager/stationsmanager.h
  stations/manager/stationjobview.h

  stations/manager/stationplanmodel.cpp
  stations/manager/stationsmanager.cpp
  stations/manager/stationjobview.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  stations/manager/stationsmanager.ui
  stations/manager/stationjobview.ui
  PARENT_SCOPE
)
