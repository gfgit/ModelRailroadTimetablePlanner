add_subdirectory(jobeditor)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  jobs/jobssqlmodel.h
  jobs/jobssqlmodel.cpp
  jobs/jobstorage.h
  jobs/jobstorage.cpp
  jobs/traingraphics.h
  jobs/traingraphics.cpp
  jobs/jobsmanager.h
  jobs/jobsmanager.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
