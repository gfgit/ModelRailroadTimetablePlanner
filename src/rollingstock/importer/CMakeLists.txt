add_subdirectory(backends)
add_subdirectory(intefaces)
add_subdirectory(model)
add_subdirectory(pages)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  rollingstock/importer/rsimportstrings.h
  rollingstock/importer/rsimportwizard.h

  rollingstock/importer/rsimportwizard.cpp
  PARENT_SCOPE
)
