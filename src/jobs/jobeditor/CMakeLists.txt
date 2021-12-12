add_subdirectory(model)
add_subdirectory(shiftbusy)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}

  jobs/jobeditor/jobpatheditor.h
  jobs/jobeditor/rscoupledialog.h
  jobs/jobeditor/stopdelegate.h
  jobs/jobeditor/stopeditinghelper.h
  jobs/jobeditor/stopeditor.h
  jobs/jobeditor/editstopdialog.h

  jobs/jobeditor/jobpatheditor.cpp
  jobs/jobeditor/rscoupledialog.cpp
  jobs/jobeditor/stopdelegate.cpp
  jobs/jobeditor/stopeditinghelper.cpp
  jobs/jobeditor/stopeditor.cpp
  jobs/jobeditor/editstopdialog.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  jobs/jobeditor/editstopdialog.ui
  jobs/jobeditor/jobpatheditor.ui
  PARENT_SCOPE
)
