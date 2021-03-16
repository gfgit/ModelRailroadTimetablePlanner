add_subdirectory(model)
add_subdirectory(shiftbusy)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}

  jobs/jobeditor/jobpatheditor.h
  jobs/jobeditor/rscoupledialog.h
  jobs/jobeditor/stopdelegate.h
  jobs/jobeditor/stopeditor.h
  jobs/jobeditor/editstopdialog.h

  jobs/jobeditor/jobpatheditor.cpp
  jobs/jobeditor/rscoupledialog.cpp
  jobs/jobeditor/stopdelegate.cpp
  jobs/jobeditor/stopeditor.cpp
  jobs/jobeditor/editstopdialog.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  jobs/jobeditor/editstopdialog.ui
  jobs/jobeditor/jobpatheditor.ui
  PARENT_SCOPE
)
