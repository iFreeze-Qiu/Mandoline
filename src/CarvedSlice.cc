#include <ostream>
#include "CarvedSlice.hh"


void CarvedSlice::svgPathWithSizeAndOffset(ostream &os, double width, double height, double dx, double dy, double strokeWidth)
{
    BGL::SVG svg(width, height);
    svg.header(os);

    CompoundRegions::iterator rit;
    for (rit = shells.begin(); rit != shells.end(); rit++) {
        os << "<path class=\"inset_shell\" fill=\"none\" stroke=\"black\"";
        os << " stroke-width=\"" << (strokeWidth/3.937) << "mm\"";
        os << " d=\"" << rit->svgPathWithOffset(dx,dy) << "\" />" << endl;
    }

    Paths::const_iterator pit;
    for (pit = infill.begin(); pit != infill.end(); pit++) {
        os << "<path class=\"infill_path\" fill=\"none\" stroke=\"black\"";
        os << " stroke-width=\"" << (strokeWidth/3.937) << "mm\"";
        os << " d=\"" << pit->svgPathWithOffset(dx,dy) << "\" />" << endl;
    }

    os << "<path class=\"model_outline\" fill=\"none\" stroke=\"#ff0000\"";
    os << " stroke-width=\"" << 0.05 << "mm\"";
    os << " d=\"" << perimeter.svgPathWithOffset(dx,dy) << "\" />" << endl;

    os << "<path class=\"infill_mask\" fill=\"none\" stroke=\"#00ff00\"";
    os << " stroke-width=\"" << 0.05 << "mm\"";
    os << " d=\"" << infillMask.svgPathWithOffset(dx,dy) << "\" />" << endl;

    os << "<path class=\"support_mask\" fill=\"none\" stroke=\"#7777ff\"";
    os << " stroke-width=\"" << 0.05 << "mm\"";
    os << " d=\"" << supportMask.svgPathWithOffset(dx,dy) << "\" />" << endl;

    svg.footer(os);
}


// vim: set ts=4 sw=4 nowrap expandtab: settings

