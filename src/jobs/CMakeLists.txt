add_subdirectory(jobeditor)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
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

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  PARENT_SCOPE
)
