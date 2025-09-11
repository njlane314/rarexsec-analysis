#ifndef HISTOGRAM_CUT_H
#define HISTOGRAM_CUT_H

namespace analysis {

enum class CutDirection { GreaterThan, LessThan };

struct Cut {
    double threshold;
    CutDirection direction;
};

}

#endif
