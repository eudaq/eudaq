#pragma once

#include "HidraXdcEvent.hh"
#include "HidraFersEvent.hh"
#include "HidraTrackerEvent.hh"
#include "HidraEventMeta.hh"

struct HidraEvent {
    HidraXdcEvent     xdc;
    HidraFersEvent    fers;
    HidraFersEvent    maxicc; // MAXICC crystal calorimeter: FERS-like, its own 3 boards (det_id 4)
    HidraTrackerEvent tracker;
    HidraEventMeta    meta;
};
