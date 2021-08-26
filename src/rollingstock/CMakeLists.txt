add_subdirectory(importer)
add_subdirectory(manager)
add_subdirectory(rs_checker)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  rollingstock/rollingstockmatchmodel.h
  rollingstock/rollingstocksqlmodel.h
  rollingstock/rsmatchmodelfactory.h
  rollingstock/rsmodelsmatchmodel.h
  rollingstock/rsmodelssqlmodel.h
  rollingstock/rsownersmatchmodel.h
  rollingstock/rsownerssqlmodel.h

  rollingstock/rollingstockmatchmodel.cpp
  rollingstock/rollingstocksqlmodel.cpp
  rollingstock/rsmatchmodelfactory.cpp
  rollingstock/rsmodelsmatchmodel.cpp
  rollingstock/rsmodelssqlmodel.cpp
  rollingstock/rsownersmatchmodel.cpp
  rollingstock/rsownerssqlmodel.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
