#ifndef WORKER_EVENT_TYPES_H
#define WORKER_EVENT_TYPES_H

#include <QEvent>

//Here we define custom QEvent types in a central place to avoid conflicts

enum class CustomEvents
{
    //Searchbox
    SearchBoxResults = QEvent::User + 1,

    //RS error checker
    RsErrWorkerProgress,
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
    StationLinesListModelResult,
    LinesModelResult,
    RailwayNodeModelResult,

    //Jobs
    JobsModelResult
};

#endif // WORKER_EVENT_TYPES_H
