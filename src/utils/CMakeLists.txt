add_subdirectory(delegates)
add_subdirectory(files)
add_subdirectory(localization)
add_subdirectory(thread)
add_subdirectory(wizard)

set(MR_TIMETABLE_PLANNER_SOURCES
  ${MR_TIMETABLE_PLANNER_SOURCES}
  utils/jobcategorystrings.h
  utils/owningqpointer.h
  utils/rs_types_names.h
  utils/session_rs_modes.h
  utils/types.h
  utils/model_roles.h
  utils/model_mode.h
  utils/worker_event_types.h

  utils/rs_utils.h
  utils/rs_utils.cpp

  PARENT_SCOPE
)
