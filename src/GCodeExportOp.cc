//
//  GCodeExportOp.cc
//  Mandoline
//
//  Created by GM on 11/24/10.
//  Copyright 2010 Belfry DevWorks. All rights reserved.
//

#include <BGL.hh>
#include "GCodeExportOp.hh"

#include <iostream>
#include <fstream>



GCodeExportOp::GCodeExportOp(SlicingContext* ctx, const string &file)
    : Operation(), context(ctx), fileName(file)
{
}



GCodeExportOp::~GCodeExportOp()
{
}



ostream &pathToGcode(Path& path, double zHeight, double layerThick, double speedMult, int tool, SlicingContext &ctx, ostream& out)
{
    if (path.size() == 0) {
        return out;
    }

    Lines::iterator line = path.segments.begin();
    double feed = ctx.feedRateForWidth(tool, line->extrusionWidth, layerThick) * speedMult;

    out.setf(ios::fixed);
    out.precision(2);

    // Move to the starting position...
    out << "G1 X" << line->startPt.x
        << " Y" << line->startPt.y
        << " Z" << zHeight
        << " F" << feed
        << endl;

    double filamentFeed = ctx.filamentFeedRate[tool] * speedMult;
    double retractionRate = ctx.retractionRate[tool];

    // undo reversal
    out << "M108 R" << retractionRate << " T" << tool << " (extrusion speed)" << endl;
    out << "M101 T" << tool << " (extrusion forward)" << endl;
    out.precision(0);
    out << "G4 P" << ctx.pushBackTime[tool] << " (Pause)" << endl;
    out.precision(2);

    // set filament speed
    out << "M108 R" << filamentFeed << " T" << tool << " (extrusion speed)" << endl;

    // start extruding
    out << "M101 T" << tool << " (extrusion forward)" << endl;

    for ( ; line != path.segments.end(); line++) {
        feed = ctx.feedRateForWidth(tool, line->extrusionWidth, layerThick) * speedMult;

        // Move to next point in polyline
        out << "G1 X" << line->endPt.x
            << " Y" << line->endPt.y
            << " Z" << zHeight
            << " F" << feed
            << endl;
    }

    // reversal
    out << "M102 T" << tool << " (extrusion backward)" << endl;
    out << "M108 R" << retractionRate << " T" << tool << " (extrusion speed)" << endl;
    out.precision(0);
    out << "G4 P" << ctx.retractionTime[tool] << " (Pause)" << endl;
    out.precision(2);

    // stop extruding
    out << "M103 T" << tool << " (extrusion stop)" << endl;

    return out;
}



ostream &pathsToGcode(Paths& paths, double zHeight, double layerThick, double speedMult, int tool, SlicingContext &ctx, ostream& out) {
    // TODO: inner paths?
    Paths::iterator pit;
    for ( pit = paths.begin(); pit != paths.end(); pit++) {
        pathToGcode((*pit), zHeight, layerThick, speedMult, tool, ctx, out);
    }

    return out;
}



ostream &simpleRegionsToGcode(SimpleRegions& regions, double zHeight, double layerThick, double speedMult, int tool, SlicingContext ctx, ostream& out) {
    SimpleRegions::iterator perimeter;
    for (
        perimeter = regions.begin();
        perimeter != regions.end();
        perimeter++
    ) {
        pathToGcode(perimeter->outerPath, zHeight, layerThick, speedMult, tool, ctx, out);

        // TODO: inner paths?
        pathsToGcode(perimeter->subpaths, zHeight, layerThick, speedMult, tool, ctx, out);
    }

    return out;
}



void GCodeExportOp::main()
{
    if ( isCancelled ) return;
    if ( NULL == context ) return;

    int mainTool = context->mainTool;
    int supportTool = context->supportTool;
    int hbpTool = context->hbpTool;

    context->processedLayers = 0;

    list<CarvedSlice>::iterator it;

    fstream fout;
    fout.open(fileName.c_str(), fstream::out | fstream::trunc);
    if (!fout.good()) {
        return;
    }

    fout.setf(ios::fixed);
    fout.precision(0);
    fout << "G21 (Metric)" << endl;

    fout << "G90 (Absolute positioning)" << endl;

    fout << "M107 T" << mainTool
         << " (Fan off)" << endl;

    fout << "M103 T" << mainTool
         << " (Extruder motor off)" << endl;

    fout << "M104 T" << mainTool
         << " S" << context->extruderTemp[mainTool]
         << " (Set extruder temp)" << endl;

    if (supportTool != mainTool) {
        fout << "M103 T" << supportTool << " (Extruder motor off)" << endl;

        fout << "M104 T" << supportTool
             << " S" << context->extruderTemp[supportTool]
             << " (Set extruder temp)" << endl;

        fout << "G10 L2 P0 X" << context->xAxisOffset[mainTool]
             << " Y" << context->yAxisOffset[mainTool]
             << " (Set offset for tool0)" << endl;

        fout << "G10 L2 P1 X" << context->xAxisOffset[supportTool]
             << " Y" << context->yAxisOffset[supportTool]
             << " (Set offset for tool0)" << endl;
    }

    fout << "M109 T" << hbpTool
         << " S" << context->platformTemp
         << " (Set platform temp)" << endl;

    fout << "G92 X0 Y0 Z0 (Zero our position out.)" << endl;
    fout << "G0 X-10 Y-10 Z5 (move back to make sure homing doesnt fail)" << endl;

    // TODO: Remove this.  It's specific to the belfry fabber machine.
    fout << "G161 Z (Home Z axis to bottom.)" << endl;
    fout << "G92 X0 Y0 Z0  (Zero our position out.)" << endl;
    fout << "G0 Z5 (Lift Z axis 5 mm)" << endl;
    fout << "G162 X Y (Home X and Y axes to XY+ max)" << endl;
    fout << "G92 X85 Y95 Z5  (Recalibrate positions.)" << endl;
    fout << "G0 X0 Y0 (Move to center.)" << endl;
    fout << "G0 Z0.5" << endl;
    // ======================================================

    fout << "M6 T" << mainTool
         << " (Wait for main tool to heat up)" << endl;

    if (supportTool != mainTool) {
        fout << "M6 T" << supportTool
             << " (Wait for support tool to heat up)" << endl;
    }

    if (hbpTool != mainTool && hbpTool != supportTool) {
        fout << "M6 T" << hbpTool
             << " (Wait for build platform to heat up)" << endl;
    }

    bool firstRaftLayerPrinted = false;
    bool firstMainLayerPrinted = false;

    // For each layer....
    for (it = context->slices.begin(); it != context->slices.end(); it++) {
        fout << "(new layer)" << endl;

        CarvedSlice* slice = &(*it);

        int layerTool = slice->isRaft? supportTool : mainTool;

        if (slice->isRaft) {
            if (firstRaftLayerPrinted == false) {
                if (supportTool != mainTool) {
                    fout << "G55 (swap to support tool's offsets)" << endl;
                }
                firstRaftLayerPrinted = true;
            }
        } else {
            if (firstMainLayerPrinted == false) {
                if (supportTool != mainTool) {
                    fout << "G54 (swap to main tool's offsets)" << endl;
                }
                firstMainLayerPrinted = true;
                firstRaftLayerPrinted = true;
            }
        }

        fout << "(perimeter)" << endl;
        CompoundRegions::reverse_iterator cit;
        for (cit = slice->shells.rbegin(); cit != slice->shells.rend(); cit++) {
            simpleRegionsToGcode(cit->subregions, slice->zLayer, slice->layerThickness, slice->speedMult, layerTool, *context, fout);
        }

        fout << "(infill)" << endl;
        pathsToGcode(slice->infill, slice->zLayer, slice->layerThickness, slice->speedMult, layerTool, *context, fout);

        if (slice->supportPaths.size() > 0) {
            fout << "(support)" << endl;
            if (supportTool != mainTool && !slice->isRaft) {
                fout << "G55 (swap to support tool's offsets)" << endl;
            }
            pathsToGcode(slice->supportPaths, slice->zLayer, slice->layerThickness, slice->speedMult, supportTool, *context, fout);
            if (supportTool != mainTool && !slice->isRaft) {
                fout << "G54 (swap to main tool's offsets)" << endl;
            }
        }

        context->processedLayers++;
    }

    fout << "M109 T" << hbpTool  << " S0 (Platform heater off)" << endl;
    fout << "M104 T" << mainTool << " S0 (Extruder heater off)" << endl;
    fout << "M103 T" << mainTool << " (Extruder motor off)" << endl;
    if (supportTool != mainTool) {
        fout << "M104 S0 T" << supportTool << " (Extruder heater off)" << endl;
        fout << "M103 T" << supportTool << " (Extruder motor off)" << endl;
    }
    fout << "G91 (incremental mode)" << endl;
    fout << "G0 Z1.0 (raise head slightly)" << endl;
    fout << "G90 (absolute mode)" << endl;
    fout << "G0 X0.0 Y50.0 (Move platform forward)" << endl;
    fout << "M18 (turn off steppers)" << endl;

    fout.close();

    if ( isCancelled ) return;
}


// vim: set ts=4 sw=4 nowrap expandtab: settings

