set(TRAINTIMETABLE_SOURCES
  ${TRAINTIMETABLE_SOURCES}
  printing/fileoptionspage.h
  printing/printdefs.h
  printing/printeroptionspage.h
  printing/printwizard.h
  printing/printworker.h
  printing/progresspage.h
  printing/selectionpage.h

  printing/fileoptionspage.cpp
  printing/printeroptionspage.cpp
  printing/printwizard.cpp
  printing/printworker.cpp
  printing/progresspage.cpp
  printing/selectionpage.cpp
  PARENT_SCOPE
)

set(TRAINTIMETABLE_UI_FILES
  ${TRAINTIMETABLE_UI_FILES}
  printing/pdfoptionspage.ui
  PARENT_SCOPE
)
