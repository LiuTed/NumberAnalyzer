#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dlfcn.h>
#include <errno.h>

#include "analyzer.h"

const char *usage = "Usage: analyzer [-hdDasv] [-f SOURCE] [-m METHOD[,METHOD[...]]] [NUM1 [NUM2 [...]]]\n"
"\t-h: show this message and exit\n"
"\t-d: enable default methods\n"
"\t-D: show default methods and exit\n"
"\t-a: read numbers in argument list (default)\n"
"\t-f: specify source of input, use \'-\' to read from stdin\n"
"\t-s: soft mode, ignore unavailable method, must appear before -m\n"
"\t-v: show title, vv to print detailed log\n"
"\t-m: enable methods in the method list, duplicated method will be ignored.\n"
"\t\tMethod will be searched in default list and extern.so by default.\n"
"\t\tOr use LIB:METHOD to specify the library to search"
;

struct callbackList
{
    char* name;
    struct callback *val;
    struct callbackList *next;
} *head, *tail;

void * dlHandle[128] = {NULL};
int nDlOpen = 0;
int verbose = 0;

#define logError(fmt, ...)\
    fprintf(stderr, "[ Error ]: " fmt "\n",##__VA_ARGS__)

#define log(fmt, ...)\
    do\
    {\
        if(verbose > 1)\
            fprintf(stderr, "[  Log  ]: " fmt "\n",##__VA_ARGS__);\
    }while(0)


void registerCallback(const char* name, struct callback *ptr)
{
    struct callbackList *node = malloc(sizeof(struct callbackList));
    node->name = strdup(name);
    node->val = ptr;
    node->next = NULL;
    if(!head)
    {
        head = tail = node;
    }
    else
    {
        tail->next = node;
        tail = node;
    }
}

void __attribute__((destructor)) exitCallback()
{
    int i;
    struct callbackList *ptr = head;

    while(ptr)
    {
        struct callbackList *tmp = ptr->next;
        free(ptr->val);
        free(ptr);
        ptr = tmp;
    }

    for(i = 0; i < nDlOpen; i++)
    {
        if(dlHandle[i]) dlclose(dlHandle[i]);
    }
}

void __attribute__((constructor)) startCallback()
{
    dlHandle[nDlOpen++] = dlopen("analyzer.so", RTLD_NOW | RTLD_LOCAL);
    if(!dlHandle[0])
    {
        logError("Cannot open analyzer.so: %s(%d)", strerror(errno), errno);
    }
}

void showDefaultList()
{
    char **ptr = dlsym(dlHandle[0], "defaultList");
    log("defaultList read successfully");
    if(!ptr)
    {
        logError("Cannot find symbol \'defaultList\', check if analyzer.so corrupted");
        exit(2);
    }
    while(*ptr)
    {
        puts(*ptr);
        ptr++;
    }
}

int enableDefaultList()
{
    char **ptr = dlsym(dlHandle[0], "defaultList");
    log("defaultList read successfully");
    if(!ptr)
    {
        logError("Cannot find symbol \'defaultList\', check if analyzer.so corrupted");
        return 1;
    }
    while(*ptr)
    {
        genFunc_t func = dlsym(dlHandle[0], *ptr);
        if(!func)
        {
            logError("Cannot find symbol %s, corrupted analyzer.so", *ptr);
            return 2;
        }
        log("%s read successfully", *ptr);
        registerCallback(*ptr, func());
    }
    return 0;
}

int openDl(char* name)
{
    if(nDlOpen >= 128)
    {
        logError("Too many opened library (>=128)");
        return 1;
    }
    void *handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
    if(!handle)
    {
        logError("Cannot open lib %s: %s(%d)", name, strerror(errno), errno);
        return 2;
    }
    log("extra dl: %d %s %p", nDlOpen, name, handle);
    dlHandle[nDlOpen++] = handle;
    return 0;
}

int findCallback(char* name)
{
    int i = 0;
    for(; i < nDlOpen; i++)
    {
        genFunc_t func = dlsym(dlHandle[i], "name");
        if(func)
        {
            log("func %s found in %d", name, i);
            registerCallback(name, func());
            return 0;
        }
    }
    logError("Cannot find symbol %s", name);
    return 1;
}

int main(int argc, char **argv)
{
    char cmd;
    extern int opterr, optind;
    extern char *optarg;
    int verbose = 0, soft = 0;
    struct callbackList *ptr;
    FILE *f = NULL;

    if(!dlHandle[0])
    {
        exit(1);
    }

    if(argc < 2)
    {
        puts(usage);
        exit(0);
    }

    opterr = 0;
    while((cmd = getopt(argc, argv, "hdDaf:m:sv::")) != -1)
    {
        switch(cmd)
        {
            case 'h':
                puts(usage);
                exit(0);
            case 'D':
                showDefaultList();
                exit(0);
            case 'd':
                if(!soft && enableDefaultList())
                {
                    exit(2);
                }
                break;
            case 'a':
                if(f) fclose(f);
                f = NULL;
                break;
            case 'f':
                if(f) fclose(f);
                if(!strcmp(optarg, "-"))
                {
                    f = stdin;
                }
                else
                {
                    f = fopen(optarg, "r");
                    if(!f)
                    {
                        logError("Cannot open file %s", optarg);
                        exit(3);
                    }
                }
                break;
            case 'v':
                verbose++;
                if(optarg)
                    verbose++;
                break;
            case 's':
                soft = 1;
                log("set to soft mode");
                break;
            case 'm':
            {
                char *tmp = strstr(optarg, ",");
                do
                {
                    char *sep;
                    if(tmp)
                    {
                        *tmp = '\0';
                        tmp++;
                    }
                    sep = strstr(optarg, ":");
                    if(sep)
                    {
                        *sep = '\0';
                        if(!soft && openDl(optarg))
                        {
                            exit(5);
                        }
                        optarg = sep+1;
                    }
                    if(!soft && findCallback(optarg))
                    {
                        exit(6);
                    }
                    optarg = tmp;
                }while(optarg);
                break;
            }
            default:
                log("unknown argument, stop parsing");
                goto _out;
        }
    }
_out:
    while(1)
    {
        char arg[32];
        double darg;
        if(f)
        {
            if(feof(f)) break;
            if(fscanf(f, "%31s", arg) == EOF)
                break;
        }
        else
        {
            if(optind >= argc) break;
            strcpy(arg, argv[optind++]);
        }
        sscanf(arg, "%lf", &darg);
        for(ptr = head; ptr; ptr = ptr->next)
        {
            if(ptr->val->iterative);
                ptr->val->iterative(darg, ptr->val->priv);
        }
    }
    for(ptr = head; ptr; ptr = ptr->next)
    {
        if(ptr->val->final)
            ptr->val->final(ptr->val->priv);
    }
    for(ptr = head; ptr; ptr = ptr->next)
    {
        if(verbose)
            printf("%11s ", ptr->name);
    }
    putc('\n', stdout);
    for(ptr = head; ptr; ptr = ptr->next)
    {
        printf("%11.5lf ", ptr->val->getResult(ptr->val->priv));
    }
    putc('\n', stdout);
    return 0;
}
