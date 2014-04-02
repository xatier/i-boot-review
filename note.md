
src/main/c_main.c

A global variable `iboot_status status;`, that records the system status of iboot

in function `c_main()`, we initialize everything


    // the status variable mentioned above
    init_status();

    // the serial port connection, baud rate: 115200, this function is defined in start_sa.S
    init_serial(SERIAL_BAUD_115200);

    init_flash();
    init_crc_table();
    init_timer();

    //////////////
    // optional //
    init_i2c ()
    init_eeprom ()
    init_eeprom_tagged()
    //////////////


    // internet and UI (serial port)
    init_ethernet(status.macaddr);
    init_ui(UI_TIMEOUT, mode_default);



    if(pcmcia(1)==0) {
       bootmem_parse("linux 0xa0008000");
    }

    // boot the linux operating system
    init_os();

    // fall back operation
    init_ui(0, mode_default);
