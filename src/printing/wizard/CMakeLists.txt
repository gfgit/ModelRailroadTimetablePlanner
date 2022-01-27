set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}

  printing/wizard/printoptionspage.h
  printing/wizard/printoptionspage.cpp

  printing/wizard/printwizard.h
  printing/wizard/printwizard.cpp

  printing/wizard/progresspage.h
  printing/wizard/progresspage.cpp

  printing/wizard/sceneselectionmodel.h
  printing/wizard/sceneselectionmodel.cpp

  printing/wizard/selectionpage.h
  printing/wizard/selectionpage.cpp

  PARENT_SCOPE
)

set(MR_TIMETABLE_PLANNER_UI_FILES
  ${MR_TIMETABLE_PLANNER_UI_FILES}
  printing/wizard/pdfoptionspage.ui
  PARENT_SCOPE
)
