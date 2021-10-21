set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  printing/fileoptionspage.h
  printing/printdefs.h
  printing/printeroptionspage.h
  printing/printwizard.h
  printing/printworker.h
  printing/progresspage.h
  printing/sceneselectionmodel.h
  printing/selectionpage.h

  printing/fileoptionspage.cpp
  printing/printeroptionspage.cpp
  printing/printwizard.cpp
  printing/printworker.cpp
  printing/progresspage.cpp
  printing/sceneselectionmodel.cpp
  printing/selectionpage.cpp
  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  printing/pdfoptionspage.ui
  PARENT_SCOPE
)
