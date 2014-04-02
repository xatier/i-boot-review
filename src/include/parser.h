//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      parser.h
//
// Description:
//
//      Handles the parsing, and execution of, incoming text commands.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      October 2001
//
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_H
#define PARSER_H

#define MAX_FILENAME_SIZE 256
#define MAX_COMMAND_NAME_SIZE  16

typedef struct
{
   char const command[MAX_COMMAND_NAME_SIZE];
   int (*parse_func)(char const *arg);
   int min_args;
} command_def;

typedef enum
{
  mode_default,
  mode_partition,
  mode_quit,
  mode_error
} mode;

mode parse_command(char const *commandline, mode cur_mode);
mode parse_script(char const *script);
int kernel_param_parse(char const *mangled, char *buf, int bufsize);
void list_commands(void);

#endif //PARSER_H
