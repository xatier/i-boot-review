//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      messages.c
//
// Description:
//
//      Functions for printing pre-defined messages.
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

#ifndef MESSAGES_H
#define MESSAGES_H

// Error number to use for an unknown error number
#define UNKNOWN_ERROR                   PARSE_MIN_ARGS_ERROR

#define PARSE_MIN_ARGS_ERROR		0
#define PARSE_NO_COMMAND_ERROR		1
#define PARSE_INVALID_IP_ERROR		2
#define PARSE_INVALID_ARG_ERROR		3
#define PARSE_RANGE_ERROR		4
#define DHCP_TIMEOUT_ERROR		5
#define TFTP_TIMEOUT_ERROR		6
#define PARSE_DL_METHOD_ERROR		7
#define PARSE_ALIGN_ERROR		8
#define PARSE_KERN_PARAM_ERROR		9
#define PARSE_INVALID_OS_ERROR		10
#define NO_OS_ERROR			11
#define FLASH_VERIFY_ERROR		12
#define FLASH_PROGRAMMING_ERROR		13
#define FLASH_PROTECTED_ERROR		14
#define FLASH_VOLTAGE_ERROR		15
#define FLASH_DEAD_ERROR		16
#define BOOTP_TIMEOUT_ERROR		17
#define CS_NOTFOUND_ERROR		18
#define CS_NOTX_ERROR			19
#define CS_ISAID_ERROR			20
#define CS_NOTCS_ERROR			21
#define CS_NOMAC_ERROR			22
#define FLASH_FLASHLOADER_ERROR		23
#define FLASH_ERASE_ERROR        	24

// Message number to use for an unknown message number
#define UNKNOWN_MESSAGE                 PARSE_DHCP_MESSAGE

#define PARSE_DHCP_MESSAGE		0
#define RAM_CLEAR_MESSAGE		1
#define LOADING_CE_MESSAGE		2
#define LOADING_LINUX_MESSAGE		3
#define PROPER_CEBIN_MESSAGE		4
#define IMPROPER_CEBIN_MESSAGE		5
#define PARSE_SET_VAR_MESSAGE		6

void error_print(int error);
void message_print(int message);

#endif //MESSAGES_H
