add_subdirectory(model)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  jobs/jobsmanager/jobsmanager.h
  jobs/jobsmanager/jobsmanager.cpp

  jobs/jobsmanager/newjobsamepathdlg.h
  jobs/jobsmanager/newjobsamepathdlg.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  PARENT_SCOPE
)
