
#include <c_main.h>
#include <timer.h>
#include <string.h>
#include <util.h>
#include <pcmcia.h>

#define DEBUG0	itc_printf

#define __REG(x) *((volatile u32 *)x)

#define MECR	__REG(0x48000014)
#define MCMEM0	__REG(0x48000028)
#define MCMEM1	__REG(0x4800002C)
#define MCATT0	__REG(0x48000030)
#define MCATT1	__REG(0x48000034)
#define MCIO0	__REG(0x48000038)
#define MCIO1	__REG(0x4800003C)

#define INTEN_BASE	0x0F400000
#define INTPOL_BASE	0x0F400004

#define S0PWR	__REG(0x0F400020)
#define S1PWR	__REG(0x0F400024)
#define RDY_CD	__REG(0x0F400028)
#define VS1_VS2	__REG(0x0F40002C)
#define RST		__REG(0x0F400034)
#define INTSTACLR	__REG(0x0F400038)

#define S1_CD	(1 << 0)
#define S0_CD	(1 << 1)
#define S1_RDY	(1 << 2)
#define S0_RDY	(1 << 3)

#define S1_RST	(1 << 2)
#define S0_RST	(1 << 3)

#define S0_CD_INT	(1 << 0)
#define S1_CD_INT	(1 << 1)
#define S0_RDY_INT	(1 << 2)
#define S1_RDY_INT	(1 << 3)

#define SZ_1M	0x100000

#define MEM_OFFSET	0xA1800000

#define read_mapping_byte(m_, o_) (m_[o_] & 0xFF)

#define IDE_STATUS_ERR  (1 << 0)
#define IDE_STATUS_DRQ  (1 << 3)
#define IDE_STATUS_DEVFLT (1 << 5)
#define IDE_STATUS_DRDY (1 << 6)
#define IDE_STATUS_BSY  (1 << 7)

static unsigned char *mem_buf = (unsigned char *)MEM_OFFSET;
static int ide_sector_buffer_stride = 2;

static struct card_info card_info[2];
static struct ide_adapter ide_ad;
static struct iohandle ioh;
static struct vfat_filesystem vfat;
static struct bootldr_status boot_status;

static int pcmcia_map_mem(u8 socket, size_t len, int cis, char **mapping)
{
    if(mapping) {
        if(socket == 0)
            *mapping = (char *)0x28000000;
        else
            *mapping = (char *)0x38000000;
        if(cis == 0)
            *mapping += 0x04000000;
        return 0;
    } else {
        return -1;
    }
}

static int pcmcia_map_io(u8 socket, size_t len, char **mapping)
{
    if(mapping) {
        *mapping = (char *)(socket ? 0x20000000 : 0x30000000);
        return 0;
    } else {
        return -1;
    }
}

static void pcmcia_init(void)
{
	S0PWR = 0;
	S1PWR = 0;

	MECR = 3;
	MCMEM0 = 0x0001020E;
	MCMEM1 = 0x0001020E;
	MCATT0 = 0x0001020E;
	MCATT1 = 0x0001020E;
	MCIO0 = 0x00014290;
	MCIO1 = 0x00014290;

	__REG(INTPOL_BASE) = 1;
	__REG(INTPOL_BASE+8) = 1;
	__REG(INTPOL_BASE+16) = 1;
	__REG(INTPOL_BASE+24) = 1;
	__REG(INTEN_BASE) = 1;
	__REG(INTEN_BASE+8) = 1;
	__REG(INTEN_BASE+16) = 1;
	__REG(INTEN_BASE+24) = 1;
	INTSTACLR = (S0_RDY_INT | S1_RDY_INT | S0_CD_INT | S1_CD_INT);
}

static int pcmcia_read_cis(int sock)
{
	volatile short *mapping = 0;
	int i = 0;
	unsigned short type, len;

	memset8((u8 *)&card_info[sock], 0, sizeof(struct card_info));
	pcmcia_map_mem(sock, SZ_1M, 1, (char **)&mapping);
//	DEBUG0("cis mapping = %x\r\n", (long)mapping);
//	DEBUG0("cis[0] = %x\r\n", read_mapping_byte(mapping, 0));
	while(i < 256) {
		type = read_mapping_byte(mapping, i++);
		len = read_mapping_byte(mapping, i++);
		if(type == CIS_TUPLE_END || len == 0) {
//			DEBUG0("end\r\n");
			break;
		}
		switch (type) {
		case CIS_TUPLE_MANFID:
			card_info[sock].manfid[0] = read_mapping_byte(mapping, i);
			card_info[sock].manfid[1] = read_mapping_byte(mapping, i+1);
//			DEBUG0("  manfid[0]= %x\r\n", card_info[sock].manfid[0]);
//			DEBUG0("  manfid[1]= %x\r\n", card_info[sock].manfid[1]);
			break;
		case CIS_TUPLE_FUNCID:
//			DEBUG0("  funcid= %x\r\n", read_mapping_byte(mapping, i));
			card_info[sock].funcid = read_mapping_byte(mapping, i);
//			if(read_mapping_byte(mapping, i) == CIS_FUNCID_FIXED)
//				DEBUG0("    fixed disk\r\n");
			break;
		}
		i += len;
	}
	return 0;
}

static int pcmcia_detect(u8 *detect)
{
	if(detect) {
		*detect = 0;
		RST |= (S0_RST | S1_RST);
		udelay(100000);
		RST &= ~(S0_RST | S1_RST);
		if(!(RDY_CD & S0_CD)) {
			*detect |= 1;
			if(INTSTACLR & S0_CD_INT) {
				INTSTACLR = S0_CD_INT;
			}
			udelay(10000);
			S0PWR = 0x0B;
			udelay(10000);
		}
		if(!(RDY_CD & S1_CD)) {
			*detect |= 2;
			if(INTSTACLR & S1_CD_INT) {
				INTSTACLR = S1_CD_INT;
			}
			udelay(10000);
			S1PWR = 0x0B;
			udelay(10000);
		}
	} else {
		return -1;
	}
	return 0;
}


//=====
/* ADD function ide_write */
int ide_write(char *buf, unsigned long offset, unsigned long len)
{
  struct drive_identification *id;
  
  if (!ide_ad.identification.n_bytes_per_sector) {
    return -1;
  } else {
//    id = &ide_ad.identification;
    
    unsigned long sector_count = len / ide_ad.identification.n_bytes_per_sector;
    unsigned long sector_number = offset / ide_ad.identification.n_bytes_per_sector;
    unsigned long start_sector = sector_number & 0xff;
    unsigned long start_cylinder = (sector_number >> 8) &0xffff ;
    unsigned long start_head = (sector_number >> 24) & 0x7 ;
    u8 c;
    int i;

    ide_ad.ioport[IDE_SECTOR_NUMBER_REG] = start_sector;
    ide_ad.ioport[IDE_CYLINDER_HIGH_REG] = (start_cylinder >> 8) & 0xff;
    ide_ad.ioport[IDE_CYLINDER_LOW_REG] = start_cylinder & 0xff;
    ide_ad.ioport[IDE_SECTOR_COUNT_REG] = sector_count & 0xff;
    ide_ad.ioport[IDE_DRIVE_HEAD_REG] = DEVICE_HEAD_IS_LBA | (start_head & 0xf);
    ide_ad.ioport[IDE_COMMAND_REG] = IDE_COMMAND_WRITE_SECTORS;
    while (ide_ad.ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
      /* wait for ready */
    }
    for (i = 0; i < 512; i += ide_sector_buffer_stride) { 
      while (!(ide_ad.ioport[IDE_STATUS_REG] & IDE_STATUS_DRQ))
	/* wait for DRQ */;
      buf[i] = buf[i] ^ '1';

      if (ide_sector_buffer_stride == 1) {
        ide_ad.ioport[IDE_SECTOR_BUF + i] = buf[i];
      }
      else {
        *(short*)&ide_ad.ioport[IDE_SECTOR_BUF + i] = *(short*)&buf[i];
      }
      while (ide_ad.ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
	/* wait for ready */
      }
    }
    c = ide_ad.ioport[IDE_STATUS_REG];
    return 0;
  }
}

/* ADD function ide_iohandle_write */
static int ide_iohandle_write(struct iohandle *ioh, char *buf, size_t offset, size_t len)
{
   struct dos_ptable_entry *pentry = (struct dos_ptable_entry *)ioh->pdata;
   unsigned long sector_size = ide_ad.identification.n_bytes_per_sector;
   unsigned long start_sector = pentry->starting_sector_lba;
   return ide_write(buf, offset + start_sector * sector_size, len);
}

/* ADD function vfat_set_next_clusterno */
static void vfat_set_next_clusterno(struct vfat_filesystem *vfat, u32 clusterno, u32 newnext)
{
    enum fat_type ftype = vfat->fat_type;
    switch (ftype) {
    case ft_fat16: {
        u16 *fat = (u16 *)vfat->fat; 
	if (newnext == VFAT_EOC) 
	  fat[clusterno] = 0xFFF8;
	else 
	  fat[clusterno] = newnext;
    } break; 
    case ft_fat32: {
        u32 *fat = (u32 *)vfat->fat;
	if (newnext == VFAT_EOC)
	  fat[clusterno] = 0x00FFFFFF;
	else
	  fat[clusterno] = newnext;
    } break;
    case ft_fat12: {
       u8 *fat = (u8 *)vfat->fat;
       u16 tempentry;
       memcpy(&tempentry, fat + (clusterno + clusterno / 2), sizeof(u16));
       if (clusterno & 1) 
          tempentry = (tempentry & 0xF) + (newnext << 4);
       else
	 tempentry = (tempentry & 0xF000) + newnext;
       memcpy((u8 *) (fat + (clusterno + clusterno/2)), &tempentry, sizeof(u16));
    } break;
    default:
    }  
}

/* ADD function vfat_count_free */
/*
 * returns the number of free clusters
 */
int vfat_count_free(struct vfat_filesystem *vfat)
{
  int i;
  int ctr = 0; 
  int n = bpb_n_clusters(&vfat->info);
  for (i = 2; i < n; i++)
    if (vfat_next_clusterno(vfat, i) == 0) ctr++;
  return ctr;
}

/* ADD function vfat_write_fat */
int vfat_write_fat(struct vfat_filesystem *vfat) {
  int offset = 0;
  int rc;
  u32 start_byte = vfat->sector_size * bpb_n_reserved_sectors(&vfat->info);
  u32 nbytes = vfat->sector_size * vfat->fat_size;
  char *buf = (char *)vfat->fat;

  for (offset = 0; offset < nbytes; offset += vfat->sector_size) {
    rc = vfat->iohandle->ops->write(vfat->iohandle, buf+offset, start_byte+offset, vfat->sector_size);
    if (rc)
      return rc;
  }
  return 0;
}

/* ADD function vfat_free_clusters */
static int vfat_free_clusters(struct vfat_filesystem *vfat, u32 first_clusterno, u8 write_fat)
{
  u32 clusterno;
  while (first_clusterno != VFAT_EOC && first_clusterno != 0) {
    clusterno = vfat_next_clusterno(vfat, first_clusterno);
    vfat_set_next_clusterno(vfat, first_clusterno, 0);
    first_clusterno = clusterno;
  }
  vfat_set_next_clusterno(vfat, first_clusterno, 0);
  if (write_fat) vfat_write_fat(vfat);
  return 0;
}

/* ADD function vfat_allocate_clusters */
/* Allocates a cluster chain efficiently */
static int vfat_allocate_clusters(struct vfat_filesystem *vfat, u32 parentcluster, struct fat_dir_entry *entry, size_t nbytes)
{
  struct bpb_info *info = &vfat->info;
  u32 first_clusterno = fat_entry_first_clusterno(entry);
  u32 n_clusters = bpb_n_clusters(info);
  u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
  u32 bytes_per_sector = bpb_bytes_per_sector(info);
  u32 bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
  u32 clusterno, next;
  size_t bytes_seen = 0;

  if (entry->n_bytes == nbytes) return first_clusterno;  /* already all allocated */
  
  if ((nbytes - 1) / bytes_per_cluster != (entry->n_bytes - 1) / bytes_per_cluster) {
    if (first_clusterno) {
      vfat_free_clusters(vfat, first_clusterno, 0);
    }
    if (vfat_count_free(vfat) < (nbytes + bytes_per_cluster - 1) / bytes_per_cluster) {
//      putstr("Not enough free space on device.\r\n");
      return -1;
    }
    first_clusterno = clusterno = 0;
    if (nbytes > 0) {
       /* go to the first data cluster */   
       for (next = 3;
	   next < n_clusters && bytes_seen <= nbytes;
	   next++)
	{
	  if (vfat_next_clusterno(vfat, next) == 0) {

	    if (first_clusterno == 0) {
	      first_clusterno = clusterno = next;
	      entry->first_cluster_high = first_clusterno >> 16;
	      entry->first_cluster_low = first_clusterno & 0xFFFF;
	    } else {
	      vfat_set_next_clusterno(vfat, clusterno, next);
	      clusterno = next;
	    }
	    bytes_seen += bytes_per_cluster;
	  }
	}
      vfat_set_next_clusterno(vfat, clusterno, VFAT_EOC);
    }
    vfat_write_fat(vfat);
  }

  return first_clusterno;
}

/* ADD function vfat_write_clusters_offset */
static int vfat_write_clusters_offset(struct vfat_filesystem *vfatt, char *buf, u32 clusterno, size_t nbytes, size_t offset)
{
	struct bpb_info *info;
	u32 sectors_per_cluster, bytes_per_sector;
	u32 bytes_per_cluster, sector_in_cluster;
	u32 first_data_sector, sector_size, sector;
	size_t bytes_read = 0, bytes_read_this_cluster;
	int rc;
	
	info = &vfatt->info;
	sectors_per_cluster = bpb_sectors_per_cluster(info);
	bytes_per_sector = bpb_bytes_per_sector(info);
	bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
	sector_in_cluster = offset / bytes_per_sector;
	first_data_sector = bpb_first_data_sector(info);
	sector_size = vfatt->sector_size;

    if(nbytes == 0)
    	return 0;
	if(offset > bytes_per_cluster)
		return -1;

	while(clusterno != VFAT_EOC && bytes_read < nbytes) {
		sector = first_data_sector + sectors_per_cluster * (clusterno - 2) + sector_in_cluster;
		bytes_read_this_cluster = sector_in_cluster * bytes_per_sector;
		while(bytes_read < nbytes && bytes_read_this_cluster < bytes_per_cluster) {
//			DEBUG0("Reading sector = %x\r\n", sector);
//			DEBUG0("Placing at = %x\r\n", (unsigned long)(buf + bytes_read));

			rc = vfatt->iohandle->ops->write(vfatt->iohandle, buf+bytes_read, sector * sector_size, bytes_per_sector);
			if (rc < 0) return rc;
			bytes_read += bytes_per_sector;
			if(bytes_read > nbytes) {
				/* do not say we have more data than was asked for */
				bytes_read = nbytes;
			}
			bytes_read_this_cluster += bytes_per_sector;
			sector++;
		}
		clusterno = vfat_next_clusterno(vfatt, clusterno);
		sector_in_cluster = 0;
    }
	if(clusterno == VFAT_EOC && bytes_read < nbytes) {
		DEBUG0("reached VFAT_EOC at bytes_read= %x\r\n", bytes_read);
	}
	return bytes_read;
}

/* ADD function vfat_write_file */
int vfat_write_file(char *buf, const char *filename, size_t nbytes)
{

    u32 first_clusterno;
    int rc;
    struct fat_dir_entry parent_storage;
    struct fat_dir_entry *parent = &parent_storage;
    struct fat_dir_entry entry_storage;
    struct fat_dir_entry *entry = &entry_storage;

    rc = vfat_find_file_entry(&vfat, parent, entry, filename);
    if (rc) 
      return rc;
    if (nbytes == 0)
      nbytes = entry->n_bytes;
	first_clusterno = vfat_allocate_clusters(&vfat, 
						   fat_entry_first_clusterno(parent), 
						   entry, nbytes);
    entry->n_bytes = vfat_write_clusters_offset(&vfat, buf, first_clusterno, nbytes, 0);

    return entry->n_bytes;
}

/* ADD function pcmcia_encrypt */
int pcmcia_encrypt(char const *infile)
{
	int /*sock,*/ bytes, flag=0;
	u8 detect=0;
	char *mapping = 0;
	int sock=1;
    
	pcmcia_init();
	udelay(10000);
	pcmcia_detect(&detect);
/*
	if(detect & 0x01) {
		sock = 0;
	}
	else if(detect & 0x02) {
		sock = 1;
	}
	else
		return -1;
*/

	if(!(detect & (sock+1))) {
		DEBUG0("Can't find %s in socket 1\r\n", infile);
		return -1;
	}

	INTSTACLR &= (S0_RDY_INT | S1_RDY_INT | S0_CD_INT | S1_CD_INT);
	delay(1);
	pcmcia_read_cis(sock); 

	if(card_info[sock].funcid == CIS_FUNCID_FIXED) {
		pcmcia_map_mem(sock, SZ_1M, 0, &mapping);
		ide_attach((volatile char *)mapping);
		udelay(10000);
		ide_read_ptable(0);
		udelay(10000);
		bytes = vfat_read_file((char *)KERNEL_RAM_START, infile, 0);

		if(bytes>0) {
			DEBUG0("%s has been read, size=%d\r\n", infile, bytes);
			
			bytes = vfat_write_file((char *)KERNEL_RAM_START, infile, bytes);
			DEBUG0("%s has been encrypted and saved, size=%d\r\n", infile, bytes);
			flag |= 0x01;
		}
    }
    return flag;//0;
}

//=====

//====================================================================
// IDE functions
//

/* Byte-swap (in 2-byte chunks) from src to dest and
   len is in bytes and counts space for a trailing NUL
   to be added to the end of dest (and is thus odd)
   */
static void copy_ident_string(char *dest, const char *src, int len)
{
    int i;

    len--;

    for (i=0;i<len;i+=2) {
        dest[i] = src[i+1];
        dest[i+1] = src[i];
    }

    dest[len] = 0;
}

static int ide_identify(void)
{
	int i;
	unsigned short *buf;
	char serial_number[21];
	char firmware_revision[9];
	char model_number[41];

	ide_ad.ioport[IDE_COMMAND_REG] = (char)IDE_COMMAND_IDENTIFY;
	while(ide_ad.ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) {
		udelay(10);
	}
	buf = (unsigned short *)&(ide_ad.identification);
	for(i=0; i < sizeof(struct drive_identification)/2; i++) {
		buf[i] = *((unsigned short *)&ide_ad.ioport[IDE_CONTROL_REG]);
		udelay(10);
	}
	copy_ident_string(serial_number, ide_ad.identification.serial_number, sizeof(serial_number));
	copy_ident_string(firmware_revision, ide_ad.identification.firmware_revision, sizeof(firmware_revision));
	copy_ident_string(model_number, ide_ad.identification.model_number, sizeof(model_number));
	DEBUG0("  serial_number: %s\r\n", serial_number);
	DEBUG0("  firmware_revision: %s\r\n", firmware_revision);
	DEBUG0("  model_number: %s\r\n", model_number);
//	DEBUG0("  n_sectors_user_addressable= %x\r\n", ide_ad.identification.n_sectors_user_addressable);
//	DEBUG0("  bytes_per_sector= %x\r\n",ide_ad.identification.n_bytes_per_sector);
//	DEBUG0("  major_version= %x\r\n", ide_ad.identification.major_version);
//	DEBUG0("  minor_version= %x\r\n", ide_ad.identification.minor_version);
//	DEBUG0("  flags= %x\r\n", ide_ad.identification.flags);
	ide_ad.identification.n_bytes_per_sector = 512;
//	DEBUG0("  ACTUAL bytes_per_sector= %x\r\n", ide_ad.identification.n_bytes_per_sector);
	return 0;
}

static int ide_attach(volatile char *ioport)
{
	memset8((u8 *)&ide_ad, 0, sizeof(struct ide_adapter));
	ide_ad.ioport = ioport;
	ide_identify();
	return 0;
}

static int ide_detach(volatile char *ioport)
{
	return 0;
}

static int ide_read(char *buf, unsigned long offset, unsigned long len)
{
	struct drive_identification *id;
	unsigned long sector_count;
	unsigned long sector_number;
	unsigned long start_sector;
	unsigned long start_cylinder;
	unsigned long start_head;
	int i;
	u8 c;

	if(!ide_ad.identification.n_bytes_per_sector) {
		return -1;
	} else {
		id = &ide_ad.identification;
		sector_count = len / id->n_bytes_per_sector;
		sector_number = offset / id->n_bytes_per_sector;
		start_sector = sector_number & 0xff;
		start_cylinder = (sector_number >> 8) & 0xffff;
		start_head = (sector_number >> 24) & 0x7;

//		DEBUG0("  start_cylinder= %x\r\n",start_cylinder);
//		DEBUG0("  start_head= %x\r\n", start_head);
//		DEBUG0("  start_sector= %x\r\n", start_sector);
//		DEBUG0("  sector_count= %x\r\n", sector_count);

		ide_ad.ioport[IDE_SECTOR_NUMBER_REG] = start_sector;
		ide_ad.ioport[IDE_CYLINDER_HIGH_REG] = (start_cylinder >> 8) & 0xff;
		ide_ad.ioport[IDE_CYLINDER_LOW_REG] = start_cylinder & 0xff;
		ide_ad.ioport[IDE_SECTOR_COUNT_REG] = sector_count & 0xff;
		ide_ad.ioport[IDE_DRIVE_HEAD_REG] = DEVICE_HEAD_IS_LBA | (start_head & 0xf);
		ide_ad.ioport[IDE_COMMAND_REG] = IDE_COMMAND_READ_SECTORS;
		while(ide_ad.ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) {
			udelay(10);
		}
		c = ide_ad.ioport[IDE_STATUS_REG];
		if(c & (IDE_STATUS_ERR | IDE_STATUS_DEVFLT)) {
			DEBUG0("ide command failed with status= %x\r\n", c);
			DEBUG0("  error reg= %x\r\n", ide_ad.ioport[IDE_ERROR_REG]);
			DEBUG0("  cyl high reg= %x\r\n", ide_ad.ioport[IDE_CYLINDER_HIGH_REG]);
			DEBUG0("  cyl low reg= %x\r\n", ide_ad.ioport[IDE_CYLINDER_LOW_REG]);
			return -1;
		}
		for(i = 0; i < 512; i += ide_sector_buffer_stride) {
			if(ide_sector_buffer_stride == 1)
				buf[i] = ide_ad.ioport[IDE_SECTOR_BUF + i];
			else
				*(short *)&buf[i] = *(short *)&ide_ad.ioport[IDE_SECTOR_BUF + i];
		}
		return 0;
	}
}

static int ide_read_ptable(struct dos_ptable_entry *ptable)
{
	unsigned char *buf = (unsigned char *)ide_ad.buf;
	int result = 0;

	result = ide_read(buf, 0, 0x200);
	if(result)
		return result;
//	DEBUG0("ptable signature= %x\r\n", (buf[511] << 8) | buf[510]);
	if(buf[510] != 0x55 || buf[511] != 0xaa) {
      DEBUG0("did not find dos partition table signature\r\n");
      return -1;
	}
	itc_memcpy((u8 *)ide_ad.ptable, &ide_ad.buf[446], 4 * sizeof(struct dos_ptable_entry));
	ide_ad.ptable_was_read = 1;
	if(ptable)
		itc_memcpy((u8 *)ptable, &ide_ad.buf[446], 4 * sizeof(struct dos_ptable_entry));
	return 0;
}

static int ide_iohandle_read(struct iohandle *iohh, char *buf, size_t offset, size_t len)
{
	struct dos_ptable_entry *pentry;
	unsigned long sector_size;
	unsigned long start_sector;

    pentry = (struct dos_ptable_entry *)iohh->pdata;
    sector_size = ide_ad.identification.n_bytes_per_sector;
    start_sector = pentry->starting_sector_lba;
	return ide_read(buf, offset + start_sector * sector_size, len);
}

static int ide_iohandle_close(struct iohandle *iohh)
{
   return 0;
}

static struct iohandle_ops ide_iohandle_ops = {
   read: ide_iohandle_read,
   write: ide_iohandle_write,
   close: ide_iohandle_close
};

struct iohandle *get_ide_partition_iohandle(u8 partid)
{
	if(!ide_ad.ptable_was_read)
		ide_read_ptable(0);
	ioh.ops = &ide_iohandle_ops;
	ioh.pdata = &ide_ad.ptable[partid];
	ioh.sector_size = ide_ad.identification.n_bytes_per_sector;
	return &ioh;
}

//====================================================================
// VFAT functions
//

static void fixup_shortname(unsigned char *dst, unsigned char *dosname)
{
	int i, j = 0;
	int has_extension = 0;
	int dot_pos = 0;
	unsigned char c;

	for(i = 0; i < 8; i++) {
		c = dosname[i];
		if(c == ' ')
			break;
		if(c >= 'A' && c <= 'Z')
			c = c + 'a' - 'A';
		dst[j++] = c;
	}
	dot_pos = j++;
	dst[dot_pos] = '.';
	for(i = 8; i < 11; i++) {
		c = dosname[i];
		if(c == ' ')
			break;
		if(c >= 'A' && c <= 'Z')
			c = c + 'a' - 'A';
		has_extension = 1;
		dst[j++] = c;
	}
	if(!has_extension)
		dst[dot_pos] = 0;
	dst[j++] = 0;
}

/* copies a segment of the name into the buffer, returns ordinal number read */
static void fixup_longname(struct fat_ldir_entry *ldir, char *buf)
{
	int offset, i;
	char *name_ptr, *dir_ptr;

	/* build the name */
	if(ldir->ord & 128) {
		/* file has been deleted */
		buf[0] = '\0';
		return;
	}
	offset = ((ldir->ord & 0x3F) - 1) * 13;
	name_ptr = &buf[offset];

	dir_ptr = (char *) (ldir) + 1;  /* skip ordinal field */
	if(offset < 0) {
		DEBUG0("Invalid long filename entry: ordinal < 0\r\n");
		return;
	}
	if(offset >= 260) {
		DEBUG0("Invalid long filename entry: filename too long\r\n");
		buf[260] = 0;
		return;
	}
	for(i = 5; i > 0; i--) {
		*name_ptr++ = *dir_ptr++;
		dir_ptr++; /* skip the empty byte of unicode */
	}

	dir_ptr += 3; /* skip flag byte, reserved field and checksum */
	for (i = 6; i > 0; i--) {
		*name_ptr++ = *dir_ptr++;
		dir_ptr++; /* skip the empty byte of unicode */
	}
	dir_ptr++; /* skip 0 field */
	dir_ptr++; /* skip 0 field */

	*name_ptr++ = *dir_ptr++;
	dir_ptr++; /* skip the empty byte of unicode */
	*name_ptr++ = *dir_ptr++; /* char 13 */
	if(ldir->ord & 0x40) {
    	*name_ptr = 0;
	}
}

static int bpb_n_fats(struct bpb_info *info)
{
	return info->n_fats;
}

static int bpb_bytes_per_sector(struct bpb_info *info)
{
	return (info->bytes_per_sector[1] << 8) + info->bytes_per_sector[0];
}

static int bpb_sectors_per_cluster(struct bpb_info *info)
{
	return info->sectors_per_cluster;
}

static int bpb_n_reserved_sectors(struct bpb_info *info)
{
	return info->n_reserved_sectors;
}

static int bpb_n_root_entries(struct bpb_info *info)
{
	return (info->n_root_entries[1] << 8) + info->n_root_entries[0];
}

static int bpb_root_dir_sectors(struct bpb_info *info)
{
	return (bpb_n_root_entries(info) * sizeof(struct fat_dir_entry) + (bpb_bytes_per_sector(info) - 1)) / bpb_bytes_per_sector(info);
}

static int bpb_fat_size(struct bpb_info *info)
{
	int fat_size = 0;

	if(info->fat_size16 != 0)
		fat_size = info->fat_size16;
	else
		fat_size = info->fat_size32;
	return fat_size;
}

static int bpb_first_root_dir_sector(struct bpb_info *info)
{
	return (bpb_n_reserved_sectors(info)
		 + (bpb_n_fats(info) * bpb_fat_size(info)));
}

static int bpb_total_sectors(struct bpb_info *info)
{
	int total_sectors = 0;

	if(info->total_sectors16[0] || info->total_sectors16[1])
		total_sectors = (info->total_sectors16[1] << 8) | info->total_sectors16[0];
	else
		total_sectors = info->total_sectors32;
	return total_sectors;
}

static int bpb_n_data_sectors(struct bpb_info *info)
{
	return bpb_total_sectors(info) -
		(bpb_n_reserved_sectors(info)
		 + (bpb_n_fats(info) * bpb_fat_size(info))
		 + bpb_root_dir_sectors(info));
}

static int bpb_first_data_sector(struct bpb_info *info)
{
	return (bpb_n_reserved_sectors(info)
		+ (bpb_n_fats(info) * bpb_fat_size(info))
		+ bpb_root_dir_sectors(info));
}

static int bpb_n_clusters(struct bpb_info *info)
{
	return bpb_n_data_sectors(info) / bpb_sectors_per_cluster(info);
}

static enum fat_type bpb_fat_type(struct bpb_info *info)
{
	u32 n_clusters = bpb_n_clusters(info);

	if(n_clusters < 4085)
    	return ft_fat12;
	else if(n_clusters < 65535)
		return ft_fat16;
	else
		return ft_fat32;
}

static int fat_entry_first_clusterno(struct fat_dir_entry *entry)
{
    return (entry->first_cluster_high << 16) | entry->first_cluster_low;
}

static u32 vfat_next_clusterno(struct vfat_filesystem *vfatt, u32 clusterno)
{
	u32 entry = 0;
	u16 *fat16;
	u32 *fat32;
	u8 *fat8;

	switch(vfatt->fat_type) {
	case ft_fat16:
		fat16 = (u16 *)vfatt->fat;
		entry = fat16[clusterno];
		if(entry >= 0xFFF8)
			entry = VFAT_EOC;
		break;
	case ft_fat32:
		fat32 = (u32 *)vfatt->fat;
		entry = fat32[clusterno];
		entry &= 0x00FFFFFF;
		if(entry >= 0x00FFFFF8)
			entry = VFAT_EOC;
		break;
	case ft_fat12:
		fat8 = (u8 *)vfatt->fat;
		itc_memcpy((u8 *)&entry, fat8 + (clusterno + clusterno/2), sizeof(u16));
		if(clusterno & 1) /* if odd entry, shift right 4 bits */
			entry >>= 4;
		else
			entry &= 0xFFF; /* else use lower 12 bits */
		break;
    default:
        return 0;
    }
    return entry;
}

static int vfat_read_bpb_info(struct vfat_filesystem *vfatt, struct iohandle *iohh)
{
	/* read bpb_info from first sector of partition */
	int rc = iohh->ops->read(iohh, vfatt->buf, 0, 512);
	if(!rc)
		itc_memcpy((u8 *)&vfatt->info, vfatt->buf, sizeof(struct bpb_info));
	return rc;
}

static int vfat_mount(struct vfat_filesystem *vfatt, struct iohandle *iohh)
{
//	char oemname[9];
	struct bpb_info *info;
	int rc = 0;
	size_t sector_size;

	memset8((u8 *)vfatt, 0, sizeof(struct vfat_filesystem));
	info = &vfatt->info;
	sector_size = iohh->sector_size;
	vfatt->iohandle = iohh;
//	DEBUG0("vfat mount: reading bpb_info\r\n");
	if(vfat_read_bpb_info(vfatt, iohh) == 0) {
//		itc_memcpy(oemname, vfatt->info.oemname, 8);
//		oemname[8] = 0x00;
//		DEBUG0("  oemname= %s\r\n", oemname);
//		DEBUG0("  sectors_per_cluster= %x\r\n", bpb_sectors_per_cluster(info));
//		DEBUG0("  n_reserved_sectors= %x\r\n", bpb_n_reserved_sectors(info));
//		DEBUG0("  n_root_entries= %x\r\n", bpb_n_root_entries(info));
//		DEBUG0("  root_dir_sectors= %x\r\n", bpb_root_dir_sectors(info));
//		DEBUG0("  first_root_dir_sector= %x\r\n", bpb_first_root_dir_sector(info));
//		DEBUG0("  fat_size= %x\r\n", bpb_fat_size(info));
//		DEBUG0("  fat_size_bytes= %x\r\n", sector_size*bpb_fat_size(info));
//		DEBUG0("  n_fats= %x\r\n", bpb_n_fats(info));
//		DEBUG0("  total_sectors= %x\r\n", bpb_total_sectors(info));
//		DEBUG0("  n_data_sectors= %x\r\n", bpb_n_data_sectors(info));
//		DEBUG0("  first_data_sector= %x\r\n", bpb_first_data_sector(info));
//		DEBUG0("  n_clusters= %x\r\n", bpb_n_clusters(info));
//		DEBUG0("  fat_type= %x\r\n", bpb_fat_type(info));

        /* gather data */
        sector_size = bpb_bytes_per_sector(info);
        vfatt->sector_size = sector_size;
        vfatt->fat_type = bpb_fat_type(info);

        /* read the root_dir_entries */
        vfatt->n_root_entries = bpb_n_root_entries(info);
        vfatt->root_dir_entries = (struct fat_dir_entry *)mem_buf;
        mem_buf += vfatt->n_root_entries * sizeof(struct fat_dir_entry);
        {
            int offset = 0;
            u32 start_byte = sector_size * bpb_first_root_dir_sector(info);
            u32 nbytes = vfatt->n_root_entries * sizeof(struct fat_dir_entry);
            char *buf = (char *)vfatt->root_dir_entries;
            for (offset = 0; offset < nbytes; offset += sector_size) {
                rc = iohh->ops->read(iohh, buf+offset, start_byte+offset, sector_size);
                if (rc)
                    return rc;
            }
        }

        /* read the fat */
        vfatt->fat_size = bpb_fat_size(info);
        vfatt->fat = mem_buf;
        mem_buf += sector_size * bpb_fat_size(info);
        {
            int offset = 0;
            u32 start_byte = sector_size * bpb_n_reserved_sectors(info);
            u32 nbytes = sector_size * bpb_fat_size(info);
            char *buf = (char *)vfatt->fat;
            for (offset = 0; offset < nbytes; offset += sector_size) {
                rc = iohh->ops->read(iohh, buf+offset, start_byte+offset, sector_size);
                if (rc)
                    return rc;
            }
        }
    }
	return rc;
}

static int vfat_mount_partition(int partid)
{
	struct bpb_info *info;
	struct iohandle *iohh = NULL;

	info = &vfat.info;
//	DEBUG0("vfat mount: partid= %x\r\n", partid);
	iohh = get_ide_partition_iohandle(partid);
	return vfat_mount(&vfat, iohh);
}

struct fat_dir_entry *vfat_find_file_in_dir(struct fat_dir_entry *dir, char *basename, int max)
{
	int use_long_name = 0;
	int ctr = 0, result;
	char longbuf[261];
	char name[15];

	while (dir->name[0] && (max == 0 || ctr < max)) {
		ctr++;
		if(dir->attr == 0) {
		/* skip if attr is zero */
		} else if(dir->name[0] == 0xe5) {
		/* skip if entry is deleted */
		} else if(dir->attr & vfat_attr_long_name) {
			use_long_name = 1;
			fixup_longname((struct fat_ldir_entry *)dir, longbuf);
		} else {
			result = 1;
			if(use_long_name) {
//				DEBUG0("find long filename: %s\r\n", longbuf);
				result = (itc_strcmp(longbuf, basename)==1) ? 0: 1;
			}
			if(result) {
				fixup_shortname(name, dir->name);
//				DEBUG0("find short filename: %s\r\n", name);
				result = (itc_strcmp(name, basename)==1) ? 0: 1;
			}
			if(result == 0) return dir;
			use_long_name = 0;
		}
		dir++;
	}
	return (struct fat_dir_entry *) 0;
}

static int vfat_find_file_entry(struct vfat_filesystem *vfatt, struct fat_dir_entry *outparent, struct fat_dir_entry *outentry, const char *fname)
{
	struct fat_dir_entry *dir;
	struct fat_dir_entry *file;

	dir = vfatt->root_dir_entries;
	file = vfat_find_file_in_dir(dir, (char *)fname, 0);
	if(file) {
		if(outentry->attr & vfat_attr_directory)
			return -1;
		itc_memcpy((u8 *)outentry, file, sizeof(struct fat_dir_entry));
		return 0;
	} else {
		DEBUG0("Could not find %s\r\n", fname);
		return -1;
	}
}

static int vfat_read_clusters_offset(struct vfat_filesystem *vfatt, char *buf, u32 clusterno, size_t nbytes, size_t offset)
{
	struct bpb_info *info;
	u32 sectors_per_cluster, bytes_per_sector;
	u32 bytes_per_cluster, sector_in_cluster;
	u32 first_data_sector, sector_size, sector;
	size_t bytes_read = 0, bytes_read_this_cluster;
	int rc;

	info = &vfatt->info;
	sectors_per_cluster = bpb_sectors_per_cluster(info);
	bytes_per_sector = bpb_bytes_per_sector(info);
	bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
	sector_in_cluster = offset / bytes_per_sector;
	first_data_sector = bpb_first_data_sector(info);
	sector_size = vfatt->sector_size;

    if(nbytes == 0)
    	return 0;
	if(offset > bytes_per_cluster)
		return -1;

	while(clusterno != VFAT_EOC && bytes_read < nbytes) {
		sector = first_data_sector + sectors_per_cluster * (clusterno - 2) + sector_in_cluster;
		bytes_read_this_cluster = sector_in_cluster * bytes_per_sector;
		while(bytes_read < nbytes && bytes_read_this_cluster < bytes_per_cluster) {
//			DEBUG0("Reading sector = %x\r\n", sector);
//			DEBUG0("Placing at = %x\r\n", (unsigned long)(buf + bytes_read));

			rc = vfatt->iohandle->ops->read(vfatt->iohandle, buf+bytes_read, sector * sector_size, bytes_per_sector);
			if (rc < 0) return rc;
			bytes_read += bytes_per_sector;
			if(bytes_read > nbytes) {
				/* do not say we have more data than was asked for */
				bytes_read = nbytes;
			}
			bytes_read_this_cluster += bytes_per_sector;
			sector++;
		}
		clusterno = vfat_next_clusterno(vfatt, clusterno);
		sector_in_cluster = 0;
    }
	if(clusterno == VFAT_EOC && bytes_read < nbytes) {
		DEBUG0("reached VFAT_EOC at bytes_read= %x\r\n", bytes_read);
	}
	return bytes_read;
}

/*
 * Reads nbytes of data from the named file into the provided buffer.
 *  If nbytes is 0, read whole file.
 */
static int vfat_read_file(char *buf, const char *filename, size_t nbytes)
{
	struct fat_dir_entry entry;
	int rc;
	u32 first_clusterno;

	vfat_mount_partition(0);
	rc = vfat_find_file_entry(&vfat, (struct fat_dir_entry *)0, &entry, filename);
	if(rc)
		return rc;
	if(nbytes == 0)
		nbytes = entry.n_bytes;
	first_clusterno = fat_entry_first_clusterno(&entry);
	return vfat_read_clusters_offset(&vfat, buf, first_clusterno, nbytes, 0);
}

//====================================================================
// PCMCIA functions
//

static int pcmcia_insert(int sock)//void)
{
	int /*sock,*/ bytes, flag=0;
	u8 detect=0;
	char *mapping = 0;

	pcmcia_init();
	udelay(10000);
	pcmcia_detect(&detect);
/*
	if(detect & 0x01) {
		sock = 0;
	}
	else if(detect & 0x02) {
		sock = 1;
	}
	else
		return -1;
*/
	if(!(detect & (sock+1)))
		return -1;

	INTSTACLR &= (S0_RDY_INT | S1_RDY_INT | S0_CD_INT | S1_CD_INT);
	delay(1);
	pcmcia_read_cis(sock);
	if(card_info[sock].funcid == CIS_FUNCID_FIXED) {
		pcmcia_map_mem(sock, SZ_1M, 0, &mapping);
		ide_attach((volatile char *)mapping);
		udelay(10000);
		ide_read_ptable(0);
		udelay(10000);
		bytes = vfat_read_file((char *)KERNEL_RAM_START, kname, 0);
		if(bytes>0) {
			boot_status.kernel_size = bytes;
			boot_status.kernel_base = (unsigned char *)KERNEL_RAM_START;
			DEBUG0("kernel has been downloaded to 0x%x, size=%d\r\n",
				boot_status.kernel_base, boot_status.kernel_size);
			flag |= 0x01;
		}
		bytes = vfat_read_file((char *)RAMDISK_RAM_START, "ramdisk.gz", 0);
		if(bytes>0) {
			boot_status.ramdisk_size = bytes;
			boot_status.ramdisk_base = (unsigned char *)RAMDISK_RAM_START;
			DEBUG0("ramdisk.gz has been downloaded to 0x%x, size=%d\r\n",
				boot_status.ramdisk_base, boot_status.ramdisk_size);
			flag |= 0x02;
		}
    }
    return flag;//0;
}

static void pcmcia_eject(void)
{
	S0PWR = 0;
	S1PWR = 0;

	__REG(INTEN_BASE) = 0;
	__REG(INTEN_BASE+8) = 0;
	__REG(INTEN_BASE+16) = 0;
	__REG(INTEN_BASE+24) = 0;

	__REG(INTPOL_BASE) = 0;
	__REG(INTPOL_BASE+8) = 0;
	__REG(INTPOL_BASE+16) = 0;
	__REG(INTPOL_BASE+24) = 0;

	INTSTACLR = (S0_RDY_INT | S1_RDY_INT | S0_CD_INT | S1_CD_INT);

	MECR = 0;

	mem_buf = (unsigned char *)MEM_OFFSET;
	DEBUG0("PCMCIA/CF card has been ejected\r\n");
}

int pcmcia(int cmd)
{
	int i;

	if(cmd==1) {
		i = pcmcia_insert(0);
		if(i<=0)	{
			DEBUG0("Can't find kernel and ramdisk in socket 0\r\n");//pcmcai insert failed\r\n");
			//return -1;
		}
		else
			return 0;
		i = pcmcia_insert(1);
		if(i<=0)	{
			DEBUG0("Can't find kernel and ramdisk in socket 1\r\n");
			return -1;
		}
		else
			return 0;
	}
	else if(cmd==0)
		pcmcia_eject();
	else
		return -1;

	return 0;
}
