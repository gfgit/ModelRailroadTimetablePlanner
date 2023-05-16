/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
