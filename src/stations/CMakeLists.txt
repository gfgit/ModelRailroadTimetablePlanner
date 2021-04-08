add_subdirectory(manager)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  stations/railwaysegmentmatchmodel.h
  stations/stationsmatchmodel.h
  stations/station_utils.h
  stations/station_name_utils.h
  stations/stationgatesmatchmodel.h
  stations/stationtracksmatchmodel.h

  stations/railwaysegmentmatchmodel.cpp
  stations/stationsmatchmodel.cpp
  stations/stationgatesmatchmodel.cpp
  stations/stationtracksmatchmodel.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  PARENT_SCOPE
)
