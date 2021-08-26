add_subdirectory(shiftgraph)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  shifts/shiftcombomodel.h
  shifts/shiftitem.h
  shifts/shiftjobsmodel.h
  shifts/shiftmanager.h
  shifts/shiftresultevent.h
  shifts/shiftsqlmodel.h
  shifts/shiftviewer.h

  shifts/shiftcombomodel.cpp
  shifts/shiftjobsmodel.cpp
  shifts/shiftmanager.cpp
  shifts/shiftresultevent.cpp
  shifts/shiftsqlmodel.cpp
  shifts/shiftviewer.cpp
  PARENT_SCOPE
)
