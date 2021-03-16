add_subdirectory(delegates)
add_subdirectory(dialogs)

set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  rollingstock/rollingstockmatchmodel.h
  rollingstock/manager/rollingstockmanager.h

  rollingstock/manager/rollingstockmanager.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  PARENT_SCOPE
)
