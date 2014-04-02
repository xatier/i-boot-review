//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      help.h
//
// Description:
//
//      This file contains the online help for I-Boot.
//
// Author:
//
//      Alfred Pang
//
// Created:
//
//      October 2001
//
////////////////////////////////////////////////////////////////////////////////

#ifndef HELP_H
#define HELP_H

#include <parser.h>

// Help data structure
typedef struct _THelpText {
   char const * cmd;
   char const * helpText;
} THelpText;

void print_help(char const *arg);

#endif //HELP_H
