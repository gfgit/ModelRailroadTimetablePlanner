add_subdirectory(common)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  odt_export/jobsheetexport.h
  odt_export/sessionrsexport.h
  odt_export/shiftsheetexport.h
  odt_export/stationsheetexport.h

  odt_export/jobsheetexport.cpp
  odt_export/sessionrsexport.cpp
  odt_export/shiftsheetexport.cpp
  odt_export/stationsheetexport.cpp
  PARENT_SCOPE
)
