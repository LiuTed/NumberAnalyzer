#include <stdlib.h>
#include <string.h>
#include "analyzer.h"

char *defaultList[] = {
    "N",
    "Mean",
    "GeoAvg",
    "Var",
    "S2Var",
    "StdVar",
    "StdS2V",
    "CV",
    "Mid",
    "Perc90",
    "Perc95",
    "Perc5",
    "Perc10",
};

typedef struct callback ret;
static ret* getret()
{
    ret *ptr = malloc(sizeof(ret));
    memset(ptr, 0, sizeof(ret));
    return ptr;
}

static void N_iter(double v, void* priv)
{
    *(int*)priv++;
}
static double N_res(void* priv)
{
    return *(int*)priv;
}
ret* N()
{
    ret *ptr = getret();
    ptr->iterative = N_iter;
    ptr->getResult = N_res;
    ptr->priv = malloc(sizeof(int));
    *(int*)ptr->priv = 0;
    return ptr;
}
