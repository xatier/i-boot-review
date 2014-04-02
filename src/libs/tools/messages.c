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

#include <string.h>
#include <util.h>
#include <messages.h>

#define MAX_ERROR_SIZE 64
#define MAX_MESSAGE_SIZE 64

// This must match the entries in message.h
const char errors[][MAX_ERROR_SIZE] = 
{
   {
      "Not enough arguments for command."
   },

   {
      "Invalid command."
   },

   {
      "Invalid IP address."
   },

   {
      "Invalid arguments."
   },

   {
      "Address out of range."
   },

   {
      "DHCP timed out."
   },

   {
      "TFTP timed out."
   },

   {
      "Unknown download method."
   },

   {
      "Address is not aligned."
   },

   {
      "Parameter string incorrectly formatted."
   },

   {
      "Invalid OS."
   },

   {
      "No OS image found."
   },

   {
      "Write verification failed."
   },

   {
      "Flash programming failure."
   },

   {
      "Flash write-protected."
   },

   {
      "Flash voltage incorrect."
   },

   {
      "Flash not responding."
   },

   {
      "Bootp timed out."
   },

   {
      "CS89x0 not found."
   },

   {
      "No TX command on CS89x0"
   },

   {
      "ISA ID does not match the CS89x0"
   },

   {
      "Chip does not identify itself as a CS89x0"
   },

   {
      "Cannot read MAC address"
   },

   {
      "Use flashloader to write below block 3."
   },

   {
      "Flash erase failure."
   }
};

// This must match the entries in message.h
const char messages[][MAX_MESSAGE_SIZE] =
{
   {
      ""
   },

   {
      "Clearing RAM..."
   },

   {
      "Loading CE..."
   },

   {
      "Loading Linux..."
   },

   {
      "Proper BIN header"
   },

   {
      "Improper BIN header"
   },

   {
      "Settable attributes: ip, gw, mask, server, speed"
   }
};

////////////////////////////////////////////////////////////////////////////////
// error_decode
// PURPOSE: Decodes an error number into the string representation.
// PARAMS:  (IN) int error - error number to decode
// RETURNS: char * - error string.
////////////////////////////////////////////////////////////////////////////////
static inline char const *
error_decode(int error)
{
      return errors[error < countof(errors) ?  error : UNKNOWN_ERROR];
}

////////////////////////////////////////////////////////////////////////////////
// error_print
// PURPOSE: Prints an error message from the array errors, based on the index,
//          error.
// PARAMS:  (IN) int error - index to print from the errors array.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
error_print(int error)
{
   itc_printf("Error: %s\r\n", error_decode(error));
}

////////////////////////////////////////////////////////////////////////////////
// message_decode
// PURPOSE: Decodes a message number into the string representation.
// PARAMS:  (IN) int message - message number to decode
// RETURNS: char * - message string.
////////////////////////////////////////////////////////////////////////////////
static inline char const *
message_decode(int message)
{
      return messages[message < countof(messages) ?  message : UNKNOWN_MESSAGE];
}

////////////////////////////////////////////////////////////////////////////////
// message_print
// PURPOSE: Prints a message from the array messages, based on the index,
//          messages.
// PARAMS:  (IN) int message - index to print from the messages array.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
message_print(int message)
{
   itc_printf("%s\r\n", message_decode(message));
}

