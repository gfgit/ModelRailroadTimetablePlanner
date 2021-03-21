add_subdirectory(manager)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  stations/stationsmatchmodel.h
  stations/stationssqlmodel.h
  stations/station_utils.h
  stations/station_name_utils.h

  stations/stationsmatchmodel.cpp
  stations/stationssqlmodel.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  PARENT_SCOPE
)
