#ifndef __ANALYZER_H__
#define __ANALYZER_H__

struct callback
{
    void (*iterative)(double v, void *priv);
    void (*final)(void *priv);
    double (*getResult)(void *priv);
    void (*destruct)();
    void *priv;
};

typedef struct callback* (*genFunc_t)(void);

#endif
