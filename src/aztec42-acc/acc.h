#ifndef ACC_H
#define ACC_H

#include <stdio.h>

#define ACC_LIMIT 128
#define ACC_PATH 1024

typedef struct {
    char *items[ACC_LIMIT];
    int count;
} Words;

typedef struct {
    int stage;
    int target;
    int model;
    int cpu;
    int floating;
    int language;
    int optimize;
    int quiet;
    int nostart;
    int nostdlib;
    int dryrun;
    int verbose;
    int save_temps;
    char *output;
    char *root;
    char *emulator;
    Words includes;
    Words definitions;
    Words libraries;
    Words inputs;
} Request;

typedef struct Command {
    Words arguments;
    Words response;
    char *product;
    char *source;
    struct Command *next;
} Command;

typedef struct {
    Command *first;
    Command *last;
    Words outputs;
    Words products;
    Words host_outputs;
    Words library_paths;
    char root[ACC_PATH];
    char cwd[ACC_PATH];
} Plan;

int fail(const char *message, const char *detail);
char *copy_text(const char *text);
int add_word(Words *words, const char *text);
void free_words(Words *words);
int parse_request(Request *request, int argc, char **argv);
void free_request(Request *request);
int build_plan(const Request *request, Plan *plan);
void print_plan(const Request *request, const Plan *plan);
void free_plan(Plan *plan);
int execute_plan(const Request *request, const Plan *plan);

#endif
