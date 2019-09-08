#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "analyzer.h"

char *defaultList[] = {
    "N",
    "Mean",
    "GeoAvg",
    "Var",
    "SVar",
    "StdVar",
    "StdSVar",
    "CV",
    "Mid",
    "Quant90",
    "Quant95",
    "Quant10",
    "Quant5",
    NULL
};

typedef struct callback ret;
static ret* getret()
{
    ret *ptr = malloc(sizeof(ret));
    memset(ptr, 0, sizeof(ret));
    return ptr;
}

static struct Statistic
{
    int N, size, capacity;
    double E, F, prod;
    double *arr;
} *statistic = NULL;

#define GetPriv(ptr) struct Statistic *stat = (struct Statistic*)priv
#define max(a,b) ((b)<(a)?(a):(b)) 

static void Basic_iter(double v, void *priv)
{
    GetPriv(priv);
    double newE = stat->E;
    stat->N += 1;
    newE += (v - newE) / stat->N;
    stat->F += (v - stat->E) * (v - newE);
    stat->E = newE;
    stat->prod *= v;
}

static void Quantile_iter(double v, void *priv)
{
    GetPriv(priv);
    if(stat->capacity <= stat->size)
    {
        stat->capacity += max(stat->capacity / 2, 1);
        stat->arr = realloc(stat->arr, stat->capacity * sizeof(double));
    }
    stat->arr[stat->size++] = v;
}

static int dCmp(const void *lhs, const void *rhs)
{
    double l = *(const double*)lhs, r = *(const double*)rhs;
    if(l < r) return -1;
    else if(l > r) return 1;
    else return 0;
}

static void Quantile_final(void *priv)
{
    GetPriv(priv);
    qsort(stat->arr, stat->size, sizeof(double), dCmp);
}

static ret* Basic(double (*result)(void*))
{
    ret *ptr = getret();
    ptr->getResult = result;
    if(statistic)
    {
        ptr->priv = statistic;
    }
    else
    {
        ptr->priv = statistic = malloc(sizeof(struct Statistic));
        memset(statistic, 0, sizeof(struct Statistic));
    }
    if(statistic->prod == 0)
    {
        ptr->iterative = Basic_iter;
        statistic->prod = 1;
    }
    return ptr;
}

static ret* Quantile(double (*result)(void*))
{
    ret *ptr = getret();
    ptr->getResult = result;
    if(statistic)
    {
        ptr->priv = statistic;
    }
    else
    {
        ptr->priv = statistic = malloc(sizeof(struct Statistic));
        memset(statistic, 0, sizeof(struct Statistic));
    }
    if(!statistic->arr)
    {
        ptr->iterative = Quantile_iter;
        ptr->final = Quantile_final;
        statistic->arr = malloc(sizeof(double));
        statistic->capacity = 1;
    }
    return ptr;
}

static double N_res(void* priv)
{
    GetPriv(priv);
    return (double)stat->N;
}
ret* N()
{
    return Basic(N_res);
}

static double Mean_res(void* priv)
{
    GetPriv(priv);
    return stat->E;
}
ret* Mean()
{
    return Basic(Mean_res);
}

static double GeoAvg_res(void* priv)
{
    GetPriv(priv);
    return pow(stat->prod, 1. / stat->N);
}
ret* GeoAvg()
{
    return Basic(GeoAvg_res);
}

static double Var_res(void* priv)
{
    GetPriv(priv);
    return stat->F / stat->N;
}
ret* Var()
{
    return Basic(Var_res);
}

static double SVar_res(void* priv)
{
    GetPriv(priv);
    return stat->F / (stat->N - 1);
}
ret* SVar()
{
    return Basic(SVar_res);
}

static double StdVar_res(void* priv)
{
    return sqrt(Var_res(priv));
}
ret* StdVar()
{
    return Basic(StdVar_res);
}

static double StdSVar_res(void* priv)
{
    return sqrt(SVar_res(priv));
}
ret* StdSVar()
{
    return Basic(StdSVar_res);
}

static double CV_res(void* priv)
{
    return StdVar_res(priv) / Mean_res(priv);
}
ret* CV()
{
    return Basic(CV_res);
}

static double Mid_res(void* priv)
{
    GetPriv(priv);
    if(stat->size & 1)
    {
        return stat->arr[stat->size >> 1];
    }
    else
    {
        return (stat->arr[stat->size / 2 - 1] + stat->arr[stat->size / 2]) / 2;
    }
    
}
ret* Mid()
{
    return Quantile(Mid_res);
}

static double Quant90_res(void* priv)
{
    GetPriv(priv);
    int idx = (stat->size * 9 + 9) / 10 - 1;
    idx = max(idx, 0);
    return stat->arr[idx];
    
}
ret* Quant90()
{
    return Quantile(Quant90_res);
}

static double Quant95_res(void* priv)
{
    GetPriv(priv);
    int idx = (stat->size * 19 + 19) / 20 - 1;
    idx = max(idx, 0);
    return stat->arr[idx];
    
}
ret* Quant95()
{
    return Quantile(Quant95_res);
}

static double Quant10_res(void* priv)
{
    GetPriv(priv);
    int idx = (stat->size + 9) / 10 - 1;
    idx = max(idx, 0);
    return stat->arr[idx];
    
}
ret* Quant10()
{
    return Quantile(Quant10_res);
}

static double Quant5_res(void* priv)
{
    GetPriv(priv);
    int idx = (stat->size + 19) / 20 - 1;
    idx = max(idx, 0);
    return stat->arr[idx];
    
}
ret* Quant5()
{
    return Quantile(Quant5_res);
}