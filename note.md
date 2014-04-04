
## system work flow

All stuffs are obviously in `src/main/c_main.c`

A global variable `iboot_status status;`, that records the system status of iboot

In function `c_main()`, we initialize everything.

A brief review in `c_main.c`


    // the status variable mentioned above
    init_status();

    // the serial port connection, baud rate: 115200, this function is defined in start_sa.S
    init_serial(SERIAL_BAUD_115200);

    init_flash();
    init_crc_table();
    init_timer();

    //////////////////////////////////////
    // optional                         //
    init_i2c ()
    init_eeprom ()
    init_eeprom_tagged()
    //////////////////////////////////////


    // internet and UI (serial port)
    init_ethernet(status.macaddr);
    init_ui(UI_TIMEOUT, mode_default);



    // this is the core of the boot loader
    if(pcmcia(1)==0) {
       bootmem_parse("linux 0xa0008000");
    }

    // boot the linux operating system
    init_os();

    // fall back operation
    init_ui(0, mode_default);



## How does `itc_printf()` work in i-boot?


- `void itc_printf(char const *format, ...)`

  - in `src/libs/tools/string.c`

  - play with `va_arg`, it invokes `output_byte_serial()` and `output_string_serial()`
  - the standard `va_start`, `va_arg` `va_end` techniques


- `output_byte_serial(char byte)`

  - in `src/libs/base/serial_sa.c`

```C
    void
    output_byte_serial(char byte)
    {
       //wait for room in the fifo.
       while((SERIAL_UTSR0 & UTSR0_TX_HALF_EMPTY) == 0);
    
       SERIAL_UTDR = byte;
    }
```


- `output_string_serial(char const *string)`

  - in `src/libs/base/serial.c`

```C
    void
    output_string_serial(char const *string)
    {
       while(*string != 0)
       {
          output_byte_serial(*string++);
          // Call idle occasionally to handle unsolicited packets
          if (*string == '\n')
             idle();
       }
    }
```


## How does Lab 2.3 work?

### Main idea

1. add a `bootk` command in `src/libs/tools/parser.c`

2. change the kernel filename read from `vfat_read_file` in `src/libs/base/pcmcia.c`

3. boot from that kernel


### work flow

1. In i-boot's UI, the command line via serial port,
user's input are sent to the command line parser,
after that the parser invokes corresponding commands.



2. In `src/lib/base/pcmica.c`, function `pcmica()` try to insert a pcmcia card,
read two files named `kernel` and `ramdisk.gz`, we change the hard coded string `kernel` to
string read from the I-Boot command line.


3. That makes we can boot from kernel we want.


