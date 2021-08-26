set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}

  stations/match_models/linesmatchmodel.h
  stations/match_models/railwaysegmentmatchmodel.h
  stations/match_models/stationgatesmatchmodel.h
  stations/match_models/stationsmatchmodel.h
  stations/match_models/stationtracksmatchmodel.h

  stations/match_models/linesmatchmodel.cpp
  stations/match_models/railwaysegmentmatchmodel.cpp
  stations/match_models/stationgatesmatchmodel.cpp
  stations/match_models/stationsmatchmodel.cpp
  stations/match_models/stationtracksmatchmodel.cpp

  PARENT_SCOPE
)
