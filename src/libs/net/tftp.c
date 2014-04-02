//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      tftp.c
//
// Description:
//
//      Handles tftp downloads and MS Platform Builder tftp uploads.
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

#include <types.h>
#include <net.h>
#include <udp.h>
#include <tftp.h>
#include <util.h>
#include <c_main.h>
#include <string.h>
#include <serial.h>
#include <timer.h>
#include <ethernet.h>
//#define DEBUG_LEVEL 4
#include <debug.h>

//#define SHOW_TFTP_BLOCKS 1

#ifdef SHOW_TFTP_BLOCKS
#define BLOCK_INDICATOR(s) itc_printf(s)
#else
#define BLOCK_INDICATOR(s) 
#endif
#define BLOCK_ERROR(s) itc_printf(s)

// Select the next block number, properly handling 16 bit overflows
#define NEXT_BLOCK(block) (((block) + 1) % 0x10000)

// States in TFTP state machine
typedef enum
{
    TFTP_ST_INIT = 0,           // Initial state (must be 0)
    TFTP_ST_FINISHED,           // Final state
    TFTP_ST_RRQ_FIRST,          // Awaiting first data packet as client
    TFTP_ST_SERVER,             // Server waiting state
    TFTP_ST_RECEIVE             // Awaiting subsequent data packets as client
} tftp_states;

// State data for the TFTP state machine
typedef struct
{
    tftp_states state;
    netapiconn conn;
    int block;
    int file_length;
    int retries;
    u8 *dest;
    char const *file_name;
    u32 start_time;
} tftp_state_data;

// Events received by TFTP state machine
typedef enum
{
    TFTP_EV_PACKET_RRQ = 1, // Received RRQ packet
    TFTP_EV_PACKET_WRQ,     // Received WRQ packet
    TFTP_EV_PACKET_DATA,    // Received DATA packet
    TFTP_EV_PACKET_ACK,     // Received ACK packet
    TFTP_EV_PACKET_ERROR,   // Received ERROR packet
    TFTP_EV_PACKET_UNKNOWN, // Received unknown packet
    TFTP_EV_BEGIN_RX,       // Begin receiving a file as a client
    TFTP_EV_BEGIN_TX,       // Begin server operation
    TFTP_EV_TIMEOUT         // Timed out
} tftp_events;

// Data associated with each event
typedef union
{
    // Data for TFTP_EV_PACKET_* events
    struct
    {
        u8 *data;
        u16 length;
    } ev_packet_x;
} tftp_ev_data;

////////////////////////////////////////////////////////////////////////////////
// tftpack :
// PURPOSE: Sends a tftp ack packet, which consists of a two byte op header
//          (TFTP_OP_ACK), and a 2 byte block number.
// PARAMS:  (IN) u16 block - tftp block # being acknowledged.
//          (IN) netapiconn conn - tftp connection used
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
tftpack(u16 block,
        netapiconn conn)
{
   u8 buf[MAX_PACKET_SIZE];
   u8 *packet = buf;
   int i;

   //we want the packet to be u32 aligned after the headers (42 bytes)
   i = (u32)(packet + ETHER_HEADER_SIZE) % sizeof(u32);
   packet += (sizeof (u32) - i);
   i = 0;

   *(u16 *)(packet + UDPIP_HEADER_SIZE + TFTP_TYPE_OFFSET) = htons(TFTP_OP_ACK);
   *(u16 *)(packet + UDPIP_HEADER_SIZE + TFTP_BLOCK_OFFSET) = htons(block);

   udp_send(&conn, packet, 2*sizeof(u16));
}

////////////////////////////////////////////////////////////////////////////////
// print_tftp_error
// PURPOSE: Display error received in TFTP ERROR packet
// PARAMS:  (IN) data - data associated with the TFTP_EV_PACKET_DATA event
// RETURNS: none
////////////////////////////////////////////////////////////////////////////////
static void
print_tftp_error(tftp_ev_data const *data)
{
    char err_msg[80];

    itc_strlcpy(err_msg,
                data->ev_packet_x.data + TFTP_ERROR_MSG_OFFSET,
                MIN(sizeof(err_msg),
                    data->ev_packet_x.length - TFTP_ERROR_MSG_OFFSET + 1));
    itc_printf("ERROR #%i received: %s\r\n",
        ntohs(*(u16 *)(data->ev_packet_x.data + TFTP_ERROR_CODE_OFFSET)),
        err_msg);
}

////////////////////////////////////////////////////////////////////////////////
// show_received
// PURPOSE: Display the number of received bytes so far
// PARAMS:  (IN) state - state data
//          (IN) force - force display no matter which block number
// RETURNS: None.
////////////////////////////////////////////////////////////////////////////////
static void
show_received(tftp_state_data const * const state, int force)
{
#ifndef SHOW_TFTP_BLOCKS
    // Display the current file length every 128 blocks starting with 1
    if (force || (state->block % 128) == 1)
    {
        itc_printf("%x\b\b\b\b\b\b\b\b", state->file_length);

    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// received_block
// PURPOSE: Store received data block into memory and update metadata
// PARAMS:  (IN/OUT) state - state data
//          (IN) data - data associated with the TFTP_EV_PACKET_DATA event
// RETURNS: 0 normally
//          1 when state machine reaches TFTP_ST_FINISHED
////////////////////////////////////////////////////////////////////////////////
static void
received_block(tftp_state_data *state, tftp_ev_data const *data)
{
    int datalen = data->ev_packet_x.length - TFTP_HEADER_SIZE;

    state->file_length += datalen;
    state->block = NEXT_BLOCK(state->block);

    DEBUG_4("%i bytes in block\r\n", datalen);
    //copy out the data payload
    itc_memcpy(state->dest,
               data->ev_packet_x.data + TFTP_HEADER_SIZE,
               datalen);
    state->dest += datalen;

    // ACK this block
    tftpack(state->block, state->conn);

    // Reset retries counter
    state->retries = 0;

    BLOCK_INDICATOR("*");
    show_received(state, 0);
}

////////////////////////////////////////////////////////////////////////////////
// ev_err
// PURPOSE: Display error message for wrong event
// PARAMS:  (IN) state - state number
//          (IN) event - event number
// RETURNS: none
////////////////////////////////////////////////////////////////////////////////
static void
ev_err(tftp_states state,  tftp_events event)
{
    itc_printf("Illegal TFTP event %i in state %i\r\n",
                (int) event, (int) state);
}

////////////////////////////////////////////////////////////////////////////////
// retry
// PURPOSE: Increment the retry counter and see if we can try again
// PARAMS:  (IN) state - state data
// RETURNS: 1 if we can try again, 0 if we must abort
////////////////////////////////////////////////////////////////////////////////
static inline int
retry(tftp_state_data *state)
{
    return ++state->retries < TFTP_RETRIES;
}

////////////////////////////////////////////////////////////////////////////////
// tftp_sm
// PURPOSE: TFTP state machine
// PARAMS:  (IN/OUT) state - state data
//          (IN) event - event into this state machine
//          (IN) data - data associated with this event
// RETURNS: 0 normally
//          1 when state machine reaches TFTP_ST_FINISHED
////////////////////////////////////////////////////////////////////////////////
static int
tftp_sm(tftp_state_data *state, tftp_events event, tftp_ev_data const *data)
{
    u8 buf[MAX_PACKET_SIZE];
    //we want the packet to be u32 aligned after the headers (42 bytes)
    u8 * const packet = (u8 *)(buf + (sizeof(u32) - (((u32)buf +\
			 UDPIP_HEADER_SIZE) % sizeof(u32))));

    DEBUG_4("Rec'd event %i in state %i\r\n", (int) event, (int) state->state);
    switch (state->state)
    {
////////////////////////////////////////////////////////////////////////////////
// Initialization state
////////////////////////////////////////////////////////////////////////////////
        case TFTP_ST_INIT:
        {
            switch (event)
            {
                case TFTP_EV_BEGIN_RX:  
                {
                    u8 *curpos = packet + UDPIP_HEADER_SIZE;
                    u16 length = 0;
                    size_t len;

                    state->start_time = get_time_timer();

                    // Get port to receive
                    udp_bind(&state->conn, 0);
 
                    // Bind address for transmitting
                    udp_connect(&state->conn, status.siaddr, IPPORT_TFTP);

                    // Build read request packet
                    *(u16 *)curpos = htons(TFTP_OP_RRQ);
                    curpos += sizeof(u16);
                    length += 2;

                    // Copy file name into file request packet
                    len = itc_strlcpy(curpos, state->file_name, TFTP_DATA_SIZE);
                    curpos += len + 1;
                    length += len + 1;
 
                    length += itc_strlcpy(curpos, "octet", TFTP_DATA_SIZE) + 1;

                    //send a TFTP read request to the server
                    udp_send(&state->conn, packet, length);
                    state->state = TFTP_ST_RRQ_FIRST;
                    break;
                }

                case TFTP_EV_BEGIN_TX:
                {
                    // Get port to receive
                    udp_bind(&state->conn, PB_PORT);
 
                    state->state = TFTP_ST_SERVER;
                    break;
                }

                default:
                {
                    // Illegal event in this state
                    ev_err(state->state, event);
                    break;
                }
            }
            break;
        }

////////////////////////////////////////////////////////////////////////////////
// Receive first block state
////////////////////////////////////////////////////////////////////////////////
        case TFTP_ST_RRQ_FIRST:
        {
            switch (event)
            {
                case TFTP_EV_PACKET_DATA:
                {
                    int const block = ntohs(*(u16 *)(data->ev_packet_x.data +
                                            TFTP_BLOCK_OFFSET));

                    // Make sure this is the packet we are expecting
                    if (NEXT_BLOCK(state->block) == block)
                    {
                        //and add it to the total received
                        received_block(state, data);

                        // Switch to regular receive state unless this is
                        // the one and only packet
                        state->state = 
                                (data->ev_packet_x.length <
                                 TFTP_DATA_SIZE + TFTP_HEADER_SIZE) ?
                                TFTP_ST_FINISHED :
                                TFTP_ST_RECEIVE;
                    }
                    else
                    {
                        BLOCK_ERROR("X");
                    }
                    // Ignore any other data blocks (should never happen)
                    break;
                }

                case TFTP_EV_TIMEOUT:  
                {
                    // Didn't receive an ACK for the RRQ
                    BLOCK_ERROR(".");
                    if (retry(state))
                    {
                        // Recursively call the state machine again to
                        // completely restart the transfer.
                        state->state = TFTP_ST_INIT;
                        tftp_sm(state, TFTP_EV_BEGIN_RX, NULL);
                    }
                    else
                    {
                        // Too many retries
                        BLOCK_ERROR("!");
                        state->state = TFTP_ST_FINISHED;
                    }
                    break;
                }

                case TFTP_EV_PACKET_ERROR:
                {
                    print_tftp_error(data);
                    state->state = TFTP_ST_FINISHED;
                    break;
                }

                default:
                {
                    // Illegal event in this state
                    ev_err(state->state, event);
                    break;
                }
            }
            break;
        }

////////////////////////////////////////////////////////////////////////////////
// Receive blocks state
////////////////////////////////////////////////////////////////////////////////
        case TFTP_ST_RECEIVE:
        {
            switch (event)
            {
                case TFTP_EV_PACKET_DATA:
                {
                    int const block = ntohs(*(u16 *)(data->ev_packet_x.data +
                                            TFTP_BLOCK_OFFSET));

                    // Make sure this is the packet we are expecting
                    if (NEXT_BLOCK(state->block) == block)
                    {
                        //and add it to the total received
                        received_block(state, data);

                        /*
                         * Here we check to see what the "actual" data size is.
                         * If we don't in fact have 512 bytes we transition to
                         * the finished state.
                         */	       
                        if (data->ev_packet_x.length <
                                TFTP_DATA_SIZE + TFTP_HEADER_SIZE)
                        {
                            state->state = TFTP_ST_FINISHED;
                        }
                    }
                    //if it's a repeat, re-acknowledge it
                    else if (state->block == block)
                    {
                        BLOCK_ERROR("+");
                        // tftp server resent packet (probably didn't get
                        // previous ack) so we ack it again

                        if (retry(state))
                        {
                            tftpack(state->block, state->conn);
                            // Stay in receive state
                        }
                        else
                        {
                            // Too many retries
                            BLOCK_ERROR("!");
                            state->state = TFTP_ST_FINISHED;
                        }
                        break;
                    }
                    else
                    {
                        // out of sync packet (this should never happen)
                        BLOCK_ERROR("X");
                        if (retry(state))
                        {
                            // Don't send ACK here
                            // Stay in receive state
                        }
                        else
                        {
                            // Too many retries
                            BLOCK_ERROR("!");
                            state->state = TFTP_ST_FINISHED;
                        }
                        break;
                    }
                    break;
                }

                case TFTP_EV_TIMEOUT:
                {
                    BLOCK_ERROR(".");
                    if (retry(state))
                    {
                        // Didn't receive our last block ACK or TFTP_ACK_WRQ
                        tftpack(state->block, state->conn);
                        // stay in receive state
                    }
                    else
                    {
                        // Too many retries
                        BLOCK_ERROR("!");
                        state->state = TFTP_ST_FINISHED;
                    }
                    break;
                }

                case TFTP_EV_PACKET_ERROR:
                {
                    print_tftp_error(data);
                    state->state = TFTP_ST_FINISHED;
                    break;
                }

                case TFTP_EV_PACKET_WRQ:
                {
                    // Hopefully, we are in server mode
                    if (state->block == 0)
                    {
                        // Must have missed our last ACK
                        BLOCK_ERROR("W");
                        tftpack(TFTP_ACK_WRQ, state->conn);
                    }
                    else
                    {
                        itc_printf("ERROR: Unexpected WRQ\r\n");
                        state->state = TFTP_ST_FINISHED;
                    }
                    break;
                }

                default:
                {
                    // Illegal event in this state
                    ev_err(state->state, event);
                    break;
                }
            }
            break;
        }

////////////////////////////////////////////////////////////////////////////////
// Server request state
////////////////////////////////////////////////////////////////////////////////
        case TFTP_ST_SERVER:
        {
            switch (event)
            {
                case TFTP_EV_PACKET_WRQ:
                {
                    state->start_time = get_time_timer();
                    BLOCK_INDICATOR("W");
                    tftpack(TFTP_ACK_WRQ, state->conn);
                    state->state = TFTP_ST_RECEIVE;
                    break;
                }

                case TFTP_EV_PACKET_ERROR:
                {
                    print_tftp_error(data);
                    state->state = TFTP_ST_FINISHED;
                    break;
                }

                case TFTP_EV_TIMEOUT:
                {
                    BLOCK_ERROR(".");
                    break;
                }

                default:
                {
                    // Illegal event in this state
                    ev_err(state->state, event);
                    break;
                }
            }
            break;
        }

////////////////////////////////////////////////////////////////////////////////
// Finished state
////////////////////////////////////////////////////////////////////////////////
        case TFTP_ST_FINISHED:
        {
            // Nothing to do!
            itc_printf("%s: We're finished!\r\n", __FUNCTION__);
            break;
        }

////////////////////////////////////////////////////////////////////////////////
// Unknown state
////////////////////////////////////////////////////////////////////////////////
        default:
        {
            // Unknown state
            ev_err(state->state, event);
            break;
        }
            
    }
    
    return state->state == TFTP_ST_FINISHED;
}

////////////////////////////////////////////////////////////////////////////////
// tftp_type_to_event
// PURPOSE: Convert a TFTP packet type into the appropriate state machine event
// PARAMS:  (IN) u16 - TFTP packet type
// RETURNS  tftp_events - event code
////////////////////////////////////////////////////////////////////////////////
static inline tftp_events
tftp_type_to_event(u16 tftp_type)
{
    switch (tftp_type)
    {
        case TFTP_OP_RRQ:       return TFTP_EV_PACKET_RRQ;
        case TFTP_OP_WRQ:       return TFTP_EV_PACKET_WRQ;
        case TFTP_OP_DATA:      return TFTP_EV_PACKET_DATA;
        case TFTP_OP_ACK:       return TFTP_EV_PACKET_ACK;
        case TFTP_OP_ERROR:     return TFTP_EV_PACKET_ERROR;
        default:                return TFTP_EV_PACKET_UNKNOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
// tftp_sm_loop
// PURPOSE: Send packet and timeout events into state machine
// PARAMS:  (IN/OUT) state - state data
//          (IN) event - initial event to send to the state machine
//                       (with NULL data pointer)
// RETURNS: file length or -1 for error
////////////////////////////////////////////////////////////////////////////////
static int
tftp_sm_loop(tftp_state_data *state, tftp_events event)
{
    u8 buf[MAX_PACKET_SIZE];
    //we want the packet to be u16 aligned after the headers (42 bytes)
    u8 * const packet = (u8 *)(buf + sizeof(u16) - 
                              (((u32)buf + UDPIP_HEADER_SIZE) % sizeof(u16)));
    u8 * const udp_data = packet + UDPIP_HEADER_SIZE;
    int tftp_timeout;
    int done = 0;
    u32 elapsed;
    tftp_ev_data data;

    /*
     * Flush the Ethernet chip of data; any data that is there now is invalid
     */
    flush_ethernet();

    // Send the initial event to the state machine
    done = tftp_sm(state, event, NULL);

    // Loop until the state machine reaches the finished state
    tftp_timeout = get_time_timer () + TFTP_TIMEOUT_PERIOD;
    while (!done)
    {
        int size;
        if ((size = udp_recvfrom(&state->conn, packet,
                TFTP_DATA_SIZE + TFTP_HEADER_SIZE + UDPIP_HEADER_SIZE,
                &state->conn)) < 0)
        {
            // Problem receiving block; may be timeout
            if (tftp_timeout < get_time_timer())
	    {
                done = tftp_sm(state, TFTP_EV_TIMEOUT, NULL);

                // Reset the timeout
                tftp_timeout = get_time_timer() + TFTP_TIMEOUT_PERIOD;
            }
        }
        else
        {
            data.ev_packet_x.data = udp_data;
            data.ev_packet_x.length = (u16) size;
            done = tftp_sm(state,
                           tftp_type_to_event(
                                ntohs(*(u16 *)(udp_data + TFTP_TYPE_OFFSET))),
                           &data);
            // Reset the timeout
            tftp_timeout = get_time_timer() + TFTP_TIMEOUT_PERIOD;
        }
    }

    /*
     * We are done, so we should clean up after ourselves.
     */
    rx_ethernet_off();
    flush_ethernet();

    show_received(state, 1);

    elapsed = MAX(1, get_time_timer() - state->start_time);
    itc_printf("\r\nDownloaded %i bytes at %i kB/s\r\n",
           state->file_length, (state->file_length / 1024) / elapsed);

    // Calling retry here actually changes the retry count, which means that
    // in the pathological case of the transfer succeeding after exactly
    // TFTP_RETRIES-1 tries, we will erroneously return failure.
    return retry(state) ? state->file_length : -1;
}

////////////////////////////////////////////////////////////////////////////////
// tftplisten
// PURPOSE: Listens for a TFTP connection to receive an MS bin file
// PARAMS:  (IN) u8 *dest - memory location to put downloaded bin file in
// RETURNS: Size of upload in bytes.
////////////////////////////////////////////////////////////////////////////////
int
tftplisten(u8 *dest)
{
    tftp_state_data state;

    itc_printf("Awaiting file send: ");

    // Initialize state variable
    memset8((u8 *)&state, 0, sizeof(state));
    //state.state = TFTP_ST_INIT;
    state.dest = dest;

    // Start the state machine
    return tftp_sm_loop(&state, TFTP_EV_BEGIN_TX);
}

////////////////////////////////////////////////////////////////////////////////
// tftpget
// PURPOSE: downloads a file via tftp. Server to download determined externally.
// PARAMS:  (IN) char * file - file to request.
//          (IN) u8 * dest   - buffer to store file.
// RETURNS  int - size of download in bytes.
////////////////////////////////////////////////////////////////////////////////
int
tftpget(char const *file,
        u8 *dest)
{
    tftp_state_data state;

    itc_printf("Downloading %s: ", file);

    // Initialize state variables
    memset8((u8 *)&state, 0, sizeof(state));
    //state.state = TFTP_ST_INIT;
    state.dest = dest;
    state.file_name = file;

    // Start the state machine
    return tftp_sm_loop(&state, TFTP_EV_BEGIN_RX);
}
