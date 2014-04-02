// Copyright Intrinsyc Software (c) 2001
// FILE: memtest.c
// PURPOSE: to test out memory.  This is for use in a bootloader, but can
//          be modified for other uses as it just takes a start and stop 
//          address for all of the checking functions

#include "types.h"
#include "memtest.h"
#include "debug.h"
#include "string.h"

#undef DEBUG
#define DEBUG(...)
//-------------------------------------------------------------------------
int do_walking_ones (unsigned int start_address, unsigned int stop_address)
{
   unsigned int addr=0,data=0, retval=0,i=0,j=0,k=0;

   DEBUG ("do_walking_ones - Starting Walking Ones Test\r\n");

   if (start_address > stop_address)
   {
      // Bad parameters
      itc_printf ("do_walking_ones - Bad Parameter Start [%08X] Stop [%08X]\r\n",
         start_address, stop_address);
      return 1;
   }

   DEBUG("do_walking_ones - Start [%08X] Stop [%08X]\r\n",
      start_address, stop_address);

   DEBUG("do_walking_ones - Starting Initial Loop\r\n");

   for (i=0;i<32;i++)
   {
      data = 1 << i;

      DEBUG("do_walking_ones - Bit Level %d\r\n",i);

      // do the writing portion.  This will actually write
      // the same data (in a walking ones) to all memory addresses
      // that were given to us.
      for (addr = start_address; addr < stop_address; addr += 4)
      {
         // This is the output status
         if ((addr % INTERVAL) == 0)
//            itc_printf("Walking Ones Write: [%08X]            \r\n",
//               addr);

         WRITE_REG (addr, data);
      }  // end of while not end of addresses
   
      DEBUG("do_walking_ones - Out of writing portion\r\n");

      // Time to start Reading portion

      for (addr = start_address; addr < stop_address; addr += 4)
      {

         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Ones Read  [%08X]           \r\n", addr);

         if (data != READ_REG (addr))
         {
            itc_printf ("walking ones failed at %08X, read %08X expected %08X\r\n",
               addr, READ_REG (addr), data);
            retval = addr;
         }
      } // end of while not end of addresses

   } // end of for loop

   DEBUG ("do_walking_ones - Done Initial Walking Ones test now for other\r\n");

   // do walking ones with different value at each memory location 
   // (walking up and down)
   for (k=0;k<=32;k++)
   {
      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Ones Write [%08X], part %2d\r\n", 
//               addr,
//               j+2);

         if (j >= 32)
            j=0;

         WRITE_REG (addr, 1 << j);
      }
   
      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Ones Read  [%08X], part %2d\r\n", 
//               addr,
//               j+2);

         if (j >= 32)
            j=0;

         if (READ_REG (addr) != 1 << j)
         {
            itc_printf ("walking ones failed at %08X read %08X expected %08X\r\n",
               addr, READ_REG (addr), data);
   
            retval = addr;
         }
      } // end of internal for loop
   } // end of different values walking

   DEBUG("do_walking_ones - Done ALL walking ones tests\r\n");
   return retval;

} // end of do_walking_ones




//-------------------------------------------------------------------------
int do_walking_zeros (unsigned int start_address, unsigned int stop_address)
{
   unsigned int addr=0, data=0, retval=0,i=0,j=0,k=0;

   if (start_address > stop_address)
   {
      // Bad parameters
      return 1;
   }

   for (i=0;i<32;i++)
   {
      data = ~(1 << i);

      DEBUG ("do_walking_zeros - Starting initial write\r\n");

      for (addr = start_address; addr < stop_address;addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Zeros Write: [%08X]           \r\n",
//               addr);

         WRITE_REG (addr,data);
      }  // end of while not end of addresses
   
      DEBUG ("do_walking_zeros - Starting initial read\r\n");

      for (addr = start_address; addr < stop_address;addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Zeros Read:  [%08X]          \r\n",
//               addr);

         if (data != READ_REG (addr))
         {
            itc_printf ("walking zeros failed at %08X read %08X expected %08X",
               addr, READ_REG (addr), data);
            retval = addr;
         }
      } // end of while not end of addresses

   } // end of for loop

   i=0;
  
   // do walking ones with different value at each memory location 
   // (walking up and down)
   for (k=0;k<32;k++)
   {
      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Zeros Write: [%08X], part %d\r\n",
//               addr,
//               j+2);

         if (j >= 32)
            j = 0;

         WRITE_REG (addr, ~(1<<j));
      }
   
      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Walking Zeros Read:  [%08X], part %d\r\n",
//               addr,
//               j+2);

         if (j >= 32)
            j = 0;

         if (READ_REG (addr) != ~(1 << j))
         {
            itc_printf ("walking zeros failed at %08X read %08X expected %08X\r\n",
               addr, READ_REG (addr), ~(1 << j));
   
            retval = addr;
         }
      }
   }
   return retval;

} // end of do_walking_zeros




//-------------------------------------------------------------------------
int do_streaming_ones (unsigned int start_address, unsigned int stop_address)
{
   unsigned int addr=0, data=0, retval=0,i=0,j=0,k=0;

   if (start_address > stop_address)
   {
      // Bad parameters
      return 1;
   }

   data = 0;

   for (i=0;i<32;i++)
   {
      data |= 1 << i;

      for (addr = start_address; addr < stop_address;addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Streaming Ones Write: [%08X]\r\n",
//               addr);

         WRITE_REG (addr, data);
      }  // end of while not end of addresses
   
      for (addr = start_address; addr < stop_address;addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Streaming Ones Read:  [%08X]\r\n",
//               addr);

         if (data != READ_REG (addr))
         {
            itc_printf ("Streaming Ones failed at %08X read %08X expected %08X\r\n",
               addr, READ_REG (addr), data);
            retval = addr;
         }
      } // end of while not end of addresses

   } // end of for loop

   i=0;

   // do walking ones with different value at each memory location 
   // (walking up and down)
   for (k=0;k<32;k++)
   {
      data = 0;
      if (k!= 0)
      {
         for (i=0;i<k;i++)
            data |= 1 << i;
      }

      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Streaming Ones Write: [%08X], part %d\r\n",
//               addr,
//               j+2);

         if ((data == MAX_UINT) || (j >= 32))
         {
            data = 0;
            j = 0;
         }

         data |= 1 << j;
         WRITE_REG (addr, data);
      }
  
      data = 0; 
      if (k!= 0)
      {
         for (i=0;i<k;i++)
            data |= 1 << i;
      }
      for (addr = start_address,j=k; addr < stop_address; j++,addr += 4)
      {
         if ((addr % INTERVAL) == 0)
//            itc_printf ("Streaming Ones Read:  [%08X], part %d\r\n",
//               addr,
//               j+2);

         if ((data == MAX_UINT) || (j >= 32))
         {
            data = 0;
            j = 0;
         }

         data |= 1 << j;

         if (READ_REG (addr) != data)
         {
            itc_printf ("Streaming Ones failed at %08X read %08X expected %08X\r\n",
               addr, READ_REG (addr), data);
   
            retval = addr;
         }
      } // for loop
   } // for loop (k one)
   return retval;
} // end of do_walking_inc







//--------------------------------------------------------------------------
int do_streaming_zeros (unsigned int start_address, unsigned int stop_address)
{
   unsigned int *p=NULL, data=0, retval=0,i=0,j=0,k=0;

   if (start_address > stop_address)
   {
      // Bad parameters
      return 1;
   }

   p = (unsigned int*)start_address;
   data = MAX_UINT;

   for (i=0;i<32;i++)
   {
      data &= ~(1 << i);

      for (p = (unsigned int*)start_address; p < (unsigned int*)stop_address;p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Streaming Zeros Write: [%08X]\r\n",
//               (unsigned int)p);

         *p = data;   
      }  // end of while not end of addresses
   
      p = (unsigned int*)start_address;

      for (p = (unsigned int*)start_address; p < (unsigned int*)stop_address;p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Streaming Zeros Read:  [%08X]\r\n",
//               (unsigned int)p);

         if (data != *p)
         {
            itc_printf ("Streaming Zeros failed at %08X read %08X expected %08X\r\n",
               (unsigned int)p, *p, data);
            retval = (unsigned int)p;
         }
      } // end of while not end of addresses

   } // end of for loop

   p = (unsigned int*)start_address;
   i=0;

   // do walking ones with different value at each memory location 
   // (walking up and down)
   for (k=0;k<32;k++)
   {
      data = MAX_UINT;
      if (k != 0)
      {
         for (i=0;i<k;i++)
            data &= ~(1<<i);
      }
      for (p = (unsigned int*)start_address,j=k; p < (unsigned int*)stop_address; j++,p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Streaming Zeros Write: [%08X], part %d\r\n",
//               (unsigned int)p,
//               j+2);

         if (j >= 32)
         {
            j=0;
            data = MAX_UINT;
         }

         data &= ~(1 << j);
         *p = data;

         if (data == 0)
         {
            data = MAX_UINT;
         }
      }
   
      data = MAX_UINT;
      if (k != 0)
      {
         for (i=0;i<k;i++)
            data &= ~(1<<i);
      }
      for (p = (unsigned int*)start_address,j=k; p < (unsigned int*)stop_address; j++,p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Streaming Zeros Read:  [%08X], part %d\r\n",
//               (unsigned int)p,
//               j+2);

         if (j >= 32)
         {
            j=0;
            data = MAX_UINT;
         }

         data &= ~(1 << j);

         if (*p != data)
         {
            itc_printf ("Streaming Zeros failed at %08X read %08X expected %08X\r\n",
               (unsigned int)p, *p, data);
   
            retval = (unsigned int)p;
         }

         if (data == 0)
         {
            data = MAX_UINT;
         }
      }
   }

   return retval;
} // end of do_walking_inc


//-------------------------------------------------------------------------
int do_number_test (unsigned int start_address, unsigned int stop_address)
{
   unsigned int *p=NULL, data=0, retval=0,i=0,j=0,k=0;

   if (start_address > stop_address)
   {
      // Bad parameters
      return 1;
   }

   p = (unsigned int*)start_address;
   data = 0;

   for (i=0;i<=MAX_UINT;i++)
   {
      data = i;

      for (p = (unsigned int*)start_address; p < (unsigned int*)stop_address;p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Number Test Write [%08X]\r\n",
//               (unsigned int)p);

         *p = data;   
      }  // end of while not end of addresses
   
      p = (unsigned int*)start_address;

      for (p = (unsigned int*)start_address; p < (unsigned int*)stop_address;p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Number Test Read:  [%08X]\r\n",
//              (unsigned int)p);

         if (data != *p)
         {
            itc_printf ("walking increment failed at %08X\r\n",
               (unsigned int)p);
            retval = (unsigned int)p;
         }
      } // end of while not end of addresses

   } // end of for loop

   p = (unsigned int*)start_address;
   i=0;

   // do walking ones with different value at each memory location 
   // (walking up and down)
   data = 0;
   for (k=0;k<=MAX_UINT;k++)
   {
      for (p = (unsigned int*)start_address,j=k; p < (unsigned int*)stop_address; j++,p++)
      {
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Number Test Write: [%08X], part %d\r\n",
//               (unsigned int)p,
//               j+2);

         data = j;
         *p = data;

         if (j == MAX_UINT)
         {
            j = 0;
         }
      }
   
      for (p = (unsigned int*)start_address,j=k; p < (unsigned int*)stop_address; j++,p++)
      { 
         if (((unsigned int)p % INTERVAL) == 0)
//            itc_printf ("Number Test Read:  [%08X], part %d\r\n",
//               (unsigned int)p,
//               j+2);

         data = j;
         if (*p != data)
         {
            itc_printf ("walking increment failed at %08X\r\n",
               (unsigned int)p);
            retval = (unsigned int)p;
         }

         if (j == MAX_UINT)
         {
            j = 0;
         }
      }

   } 

   return retval;
}
