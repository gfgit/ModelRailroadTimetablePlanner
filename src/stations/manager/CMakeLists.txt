add_subdirectory(free_rs_viewer)
add_subdirectory(railwaynode)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  stations/manager/colordelegate.h
  stations/manager/defaultplatfdelegate.h
  stations/manager/stationplanmodel.h
  stations/manager/stationsmanager.h
  stations/manager/stationjobview.h

  stations/manager/colordelegate.cpp
  stations/manager/defaultplatfdelegate.cpp
  stations/manager/stationplanmodel.cpp
  stations/manager/stationsmanager.cpp
  stations/manager/stationjobview.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  stations/manager/stationsmanager.ui
  stations/manager/stationjobview.ui
  PARENT_SCOPE
)
