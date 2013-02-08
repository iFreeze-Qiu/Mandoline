//
//  GCodeExportOp.hh
//  Mandoline
//
//  Created by GM on 11/24/10.
//  Copyright 2010 Belfry DevWorks. All rights reserved.
//

#ifndef GCODEEXPORTOP_H
#define GCODEEXPORTOP_H

#include "CarvedSlice.hh"
#include "SlicingContext.hh"
#include "Operation.hh"

class GCodeExportOp : public Operation {
public:
    SlicingContext* context;
    string fileName;

    GCodeExportOp(SlicingContext* ctx, const string &file);
    virtual ~GCodeExportOp();
    virtual void main();
};

#endif
// vim: set ts=4 sw=4 nowrap expandtab: settings

