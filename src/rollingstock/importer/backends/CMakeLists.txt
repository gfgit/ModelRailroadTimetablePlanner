add_subdirectory(ods)
add_subdirectory(sqlite)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  rollingstock/importer/backends/importmodes.h
  rollingstock/importer/backends/importtask.h
  rollingstock/importer/backends/ioptionswidget.h
  rollingstock/importer/backends/loadprogressevent.h
  rollingstock/importer/backends/loadtaskutils.h
  rollingstock/importer/backends/rsbackendsmodel.h
  rollingstock/importer/backends/rsimportbackend.h

  rollingstock/importer/backends/importtask.cpp
  rollingstock/importer/backends/ioptionswidget.cpp
  rollingstock/importer/backends/loadprogressevent.cpp
  rollingstock/importer/backends/loadtaskutils.cpp
  rollingstock/importer/backends/rsbackendsmodel.cpp
  rollingstock/importer/backends/rsimportbackend.cpp
  PARENT_SCOPE
)
