#ifndef TSKVARIABLE_H
#define TSKVARIABLE_H

#include "tsk/libtsk.h"
#include <QDebug>

struct TskObject
{
    const TSK_TCHAR** imagepartspath;
    int partcount;
    int offset;
    int length;
    TSK_IMG_INFO* readimginfo;
};
#endif // TSKVARIABLE_H
