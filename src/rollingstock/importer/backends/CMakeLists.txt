add_subdirectory(ods)
add_subdirectory(sqlite)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  rollingstock/importer/backends/importmodes.h
  rollingstock/importer/backends/importtask.h
  rollingstock/importer/backends/ioptionswidget.h
  rollingstock/importer/backends/loadprogressevent.h
  rollingstock/importer/backends/loadtaskutils.h
  rollingstock/importer/backends/optionsmodel.h

  rollingstock/importer/backends/importtask.cpp
  rollingstock/importer/backends/ioptionswidget.cpp
  rollingstock/importer/backends/loadprogressevent.cpp
  rollingstock/importer/backends/loadtaskutils.cpp
  rollingstock/importer/backends/optionsmodel.cpp
  PARENT_SCOPE
)
