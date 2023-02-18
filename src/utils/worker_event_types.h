#ifndef WORKER_EVENT_TYPES_H
#define WORKER_EVENT_TYPES_H

#include <QEvent>

//Here we define custom QEvent types in a central place to avoid conflicts

enum class CustomEvents
{
    //IQuittableTask
    TaskProgress = QEvent::User + 1,

    //Searchbox
    SearchBoxResults,

    //RS error checker
    RsErrWorkerResult,

    // RS Import
    RsImportLoadProgress,
    RsImportGoBackPrevPage,
    RsImportedRSModelResult,
    RsImportedModelsResult,
    RsImportedOwnersResult,
    RsImportCheckDuplicates,

    //RS main models
    RollingstockModelResult,
    RsModelsModelResult,
    RsOwnersModelResult,
    RsOnDemandListModelResult,

    //Shift
    ShiftWorkerResult,

    //Stations
    StationsModelResult,
    LinesModelResult,
    LineSegmentsModelResult,
    RailwaySegmentsModelResult,

    StationGatesListResult,
    StationTracksListResult,
    StationTrackConnListResult,

    //Jobs
    JobsModelResult,

    //Jobs Checker
    JobsCrossingCheckResult,

    //Printing
    PrintProgress,

    //Line Graph Manager
    LineGraphManagerUpdate
};

#endif // WORKER_EVENT_TYPES_H
