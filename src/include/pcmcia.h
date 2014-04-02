
#ifndef PCMCIA_H
#define PCMCIA_H

//==========================================================
// PCMCIA definitions
//

enum cis_tuple_type {
	CIS_TUPLE_NULL		= 0x00,
	CIS_TUPLE_DEVICE		= 0x01,
	CIS_TUPLE_LONGLINK_CB	= 0x02,
	CIS_TUPLE_INDIRECT		= 0x03,
	CIS_TUPLE_CONFIG_CB		= 0x04,
	CIS_TUPLE_CFTABLE_ENTRY_CB	= 0x05,
	CIS_TUPLE_LONGLINK_MFC	= 0x06,
	CIS_TUPLE_BAR		= 0x07,
	CIS_TUPLE_PWR_MGMNT		= 0x08,
	CIS_TUPLE_EXTDEVICE		= 0x09,
	CIS_TUPLE_CHECKSUM		= 0x10,
	CIS_TUPLE_LONGLINK_A		= 0x11,
	CIS_TUPLE_LONGLINK_C		= 0x12,
	CIS_TUPLE_LINKTARGET		= 0x13,
	CIS_TUPLE_NO_LINK		= 0x14,
	CIS_TUPLE_VERS_1		= 0x15,
	CIS_TUPLE_ALTSTR		= 0x16,
	CIS_TUPLE_DEVICE_A		= 0x17,
	CIS_TUPLE_JEDEC_C		= 0x18,
	CIS_TUPLE_JEDEC_A		= 0x19,
	CIS_TUPLE_CONFIG		= 0x1a,
	CIS_TUPLE_CFTABLE_ENTRY	= 0x1b,
	CIS_TUPLE_DEVICE_OC		= 0x1c,
	CIS_TUPLE_DEVICE_OA		= 0x1d,
	CIS_TUPLE_DEVICE_GEO		= 0x1e,
	CIS_TUPLE_DEVICE_GEO_A	= 0x1f,
	CIS_TUPLE_MANFID		= 0x20,
	CIS_TUPLE_FUNCID		= 0x21,
	CIS_TUPLE_FUNCE		= 0x22,
	CIS_TUPLE_SWIL		= 0x23,
	CIS_TUPLE_END		= 0xff
};

enum cis_funcid {
	CIS_FUNCID_MULTI	= 0x00,
	CIS_FUNCID_MEMORY	= 0x01,
	CIS_FUNCID_SERIAL	= 0x02,
	CIS_FUNCID_PARALLEL	= 0x03,
	CIS_FUNCID_FIXED	= 0x04,
	CIS_FUNCID_VIDEO	= 0x05,
	CIS_FUNCID_NETWORK	= 0x06,
	CIS_FUNCID_AIMS	= 0x07,
	CIS_FUNCID_SCSI	= 0x08
};

struct card_info {
	short manfid[2];
	short funcid;
	char *name;
};


//==========================================================
// IDE definitions
//

struct dos_ptable_entry {
	unsigned char is_active_partition;
	unsigned char starting_head;
	unsigned char starting_sector;
	unsigned char starting_cylinder;
	unsigned char partition_type;
	unsigned char ending_head;
	unsigned char ending_sector;
	unsigned char ending_cylinder;
	unsigned long starting_sector_lba;
	unsigned long n_sectors; 
};

enum ide_registers {
	IDE_DATA_REG    = 0,
	IDE_WRITE_PRECOMPENSATION_REG = 0x1,
	IDE_ERROR_REG = 1,
	IDE_FEATURE_REG = 1,
	IDE_SECTOR_COUNT_REG = 2,
	IDE_SECTOR_NUMBER_REG = 3,
	IDE_CYLINDER_LOW_REG = 4,
	IDE_CYLINDER_HIGH_REG = 5,
	IDE_DRIVE_HEAD_REG = 6,
	IDE_COMMAND_REG = 7,
	IDE_STATUS_REG  = 7,
	IDE_CONTROL_REG  = 8,
	IDE_SECTOR_BUF  = 0x400
};

/* reference: 
 * ANSI NCITS 317-1998
 * AT Attachment with Packet Interface Extension (ata/atapi-4)
 */
enum ide_command {
	IDE_COMMAND_READ_BUFFER  = 0xE4,
	IDE_COMMAND_READ_SECTORS = 0x20, /* page 139 */
	IDE_COMMAND_WRITE_SECTORS = 0x30, /* page 207 */
	IDE_COMMAND_SEEK = 0x70, /* page 157 */
	// IDE_COMMAND_ = 0x91;
	IDE_COMMAND_IDENTIFY = 0xEC,
	IDE_COMMAND_IDENTIFY_PACKET = 0xA1, /* page 93 */
};

/* set this bit if passing LBA instead of C/H/S */
#define DEVICE_HEAD_IS_LBA (1 << 6)

struct drive_identification {
  /* 0x00 */ unsigned short flags;
  /* 0x01 */ unsigned short n_cylinders;
  /* 0x02 */ unsigned short reserved02;
  /* 0x03 */ unsigned short n_heads;
  /* 0x04 */ unsigned short n_bytes_per_track;
  /* 0x05 */ unsigned short n_bytes_per_sector;
  /* 0x06 */ unsigned short n_sectors_per_track;
  /* 0x07 */ unsigned short inter_sector_gap;
  /* 0x08 */ unsigned short reserved08;
  /* 0x09 */ unsigned short n_vendor_status_words;
  /* 0x0a */ unsigned char serial_number[20];
  /* 0x14 */ unsigned short controller_type;
  /* 0x15 */ unsigned short n_buffer_512b;
  /* 0x16 */ unsigned short n_ecc_bytes;
  /* 0x17 */ unsigned char firmware_revision[8];
  /* 0x1b */ unsigned char model_number[40] ;
  /* 0x2f */ unsigned short read_write_multiples_implemented;
  /* 0x30 */ unsigned short double_word_io_supported;
  /* 0x31 */ unsigned short capabilities31;
  /* 0x32 */ unsigned short capabilities32;
  /* 0x33 */ unsigned short minimum_pio_mode_number;
  /* 0x34 */ unsigned short retired34;
  /* 0x35 */ unsigned short flags53;
  /* 0x36 */ unsigned short current_n_cylinders;
  /* 0x37 */ unsigned short current_n_heads;
  /* 0x38 */ unsigned short current_n_sectors_per_track;
  /* 0x39 */ unsigned long  current_n_sectors;
  /* 0x3a */
  /* 0x3b */ unsigned short reserved3b;
  /* 0x3c */ unsigned long n_sectors_user_addressable;
  /* 0x3d */ 
  /* 0x3e */ unsigned short retired3e;
  /* 0x3f */ unsigned short reserved3f;
  /* 0x40 - 0x4f */ unsigned short reserved40[16];
  /* 0x50 */ unsigned short major_version;
  /* 0x51 */ unsigned short minor_version;
  /* 0x52 */ unsigned short cmd_set[6];
};

struct ide_adapter {
	volatile char *ioport;
	char buf[512];
	int ptable_was_read;
	struct dos_ptable_entry ptable[16];
	struct drive_identification identification; 
};


//==========================================================
// IO Handle definitions
//

struct iohandle;
struct iohandle_ops {
	int (*read)(struct iohandle *ioh, char *buf, size_t offset, size_t len);
	int (*close)(struct iohandle *ioh);
	void *pdata; /* can point to filesystem or io device */
};

struct iohandle {
	struct iohandle_ops *ops;
	int sector_size;
	void *pdata; /* can point to particular file data */
};


//==========================================================
// VFAT definitions
//

enum fat_type {
	ft_unknown,
	ft_fat12,
	ft_fat16,
	ft_fat32
};

/* 
 * Boot Sector and BPB Structure
 *  Found at sector 0 of partition
 */
struct bpb_info {
  /* offset  0 */ u8 jmpboot[3]; /* ignore this */
  /* offset  3 */ u8 oemname[8];
  /* offset 11 */ u8 bytes_per_sector[2]; /* unaligned, so u8 [2] */
  /* offset 13 */ u8  sectors_per_cluster;
  /* offset 14 */ u16 n_reserved_sectors; 
  /* offset 16 */ u8 n_fats;  
  /* offset 17 */ u8 n_root_entries[2];
  /* offset 19 */ u8 total_sectors16[2];
  /* offset 21 */ u8 media;
  /* offset 22 */ u16 fat_size16;
  /* offset 24 */ u16 sectors_per_track;  
  /* offset 26 */ u16 n_heads;  
  /* offset 28 */ u32 n_hidden_sectors;  
  /* offset 32 */ u32 total_sectors32;

  /* following fields for fat32 only */
  /* offset 36 */ u32 fat_size32;
  /* offset 40 */ u16 ext_flags;
  /* offset 42 */ u16 fsver;
  /* offset 44 */ u32 first_root_cluster;
  /* offset 48 */ u16 fsinfo;
  /* offset 50 */ u16 backup_bootblock_sector;
  /* offset 52 */ u8 reserved52[12];
  /* offset 64 */ u8 drive_number;
  /* offset 65 */ u8 reserved65;
  /* offset 66 */ u8 bootsig;
  /* offset 67 */ u8 volume_id[4];
  /* offset 71 */ u8 volume_label[11];
  /* offset 82 */ u8 filesystem_tyep[8];
};

struct bpb_info16 {
  /* offset  0 */ u8 jmpboot[3]; /* ignore this */
  /* offset  3 */ u8 oemname[8];
  /* offset 11 */ u8 bytes_per_sector[2]; /* unaligned, so u8 [2] */
  /* offset 13 */ u8  sectors_per_cluster;
  /* offset 14 */ u16 n_reserved_sectors; 
  /* offset 16 */ u8 n_fats;  
  /* offset 17 */ u8 n_root_entries[2];
  /* offset 19 */ u8 total_sectors16[2];
  /* offset 21 */ u8 media;
  /* offset 22 */ u16 fat_size16;
  /* offset 24 */ u16 sectors_per_track;  
  /* offset 26 */ u16 n_heads;  
  /* offset 28 */ u32 n_hidden_sectors;  
  /* offset 32 */ u32 total_sectors32[2];
};

enum vfat_attribute {
	vfat_attr_read_only = 0x01,
	vfat_attr_hidden    = 0x02,
	vfat_attr_system    = 0x04,
	vfat_attr_volume_id = 0x08,
	vfat_attr_directory = 0x10,
	vfat_attr_archive   = 0x20,
	vfat_attr_long_name = 0x0f
};

struct fat_dir_entry {
  /* offset 00: */ unsigned char name[11];
  /* offset 11: */ u8 attr;
  /* offset 12: */ u8 should_be_zero;
  /* offset 13: */ u8 creation_time_tenth;
  /* offset 14: */ u16 creation_time;
  /* offset 16: */ u16 creation_date;
  /* offset 18: */ u16 last_access_date;
  /* offset 20: */ u16 first_cluster_high;
  /* offset 22: */ u16 last_write_time;
  /* offset 24: */ u16 last_write_date;
  /* offset 26: */ u16 first_cluster_low;
  /* offset 28: */ u32 n_bytes;
};

struct fat_ldir_entry {
   /* offset 00: */ u8 ord;
   /* offset 01: */ u8 name1[10]; /* unicode */
   /* offset 11: */ u8 attr;
   /* offset 12: */ u8 ldir_type;
   /* offset 13: */ u8 chksum;
   /* offset 14: */ u8 name2[12];
   /* offset 26: */ u8 must_be_zero[2];
   /* offset 28: */ u8 name3[4];
};

#define VFAT_EOC 0x00FFFFF8ul

struct vfat_filesystem {
	/* raw boot parameter block info */
	struct bpb_info info;
	/* io handle (i.e., block device) that is mounted */
	struct iohandle *iohandle;
	/* cooked info */
	u32 sector_size;
	/* fat info */ 
	enum fat_type fat_type;
	u32 fat_size;
	char *fat;
	/* root directory info */
	u32 n_root_entries;
	struct fat_dir_entry *root_dir_entries;
	char buf[4096];
};

//==========================================================
// Bootloader definitions
//

struct bootldr_status {
	unsigned char *kernel_base;
	int kernel_size;
	unsigned char *ramdisk_base;
	int ramdisk_size;
};

extern int pcmcia(int cmd);
#endif