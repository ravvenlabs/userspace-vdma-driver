// Dr. Kaputa
// VDMA control

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#define BILLION 1E9
#include <sys/types.h>
#include <sys/stat.h>
#include <byteswap.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#define PAGE_SIZE 65536
#define ARRAY_SIZE 921600
#define MAP_SIZE 2887680  // 752 * 480 * 8
#define PAGE_SIZE2 ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t)(long)~(PAGE_SIZE2 - 1))

/* Register offsets */
#define OFFSET_PARK_PTR_REG                     0x28
#define TAIL                                    0x34
#define OFFSET_VERSION                          0x2c

#define OFFSET_VDMA_MM2S_CONTROL_REGISTER       0x00
#define OFFSET_VDMA_MM2S_STATUS_REGISTER        0x04
#define OFFSET_VDMA_MM2S_VSIZE                  0x50
#define OFFSET_VDMA_MM2S_HSIZE                  0x54
#define OFFSET_VDMA_MM2S_FRMDLY_STRIDE          0x58
#define OFFSET_VDMA_MM2S_FRAMEBUFFER1           0x5c
#define OFFSET_VDMA_MM2S_FRAMEBUFFER2           0x60
#define OFFSET_VDMA_MM2S_FRAMEBUFFER3           0x64
#define OFFSET_VDMA_MM2S_FRAMEBUFFER4           0x68

#define OFFSET_VDMA_S2MM_CONTROL_REGISTER       0x30
#define OFFSET_VDMA_S2MM_STATUS_REGISTER        0x34
#define OFFSET_VDMA_S2MM_IRQ_MASK               0x3c
#define OFFSET_VDMA_S2MM_REG_INDEX              0x44
#define OFFSET_VDMA_S2MM_VSIZE                  0xa0
#define OFFSET_VDMA_S2MM_HSIZE                  0xa4
#define OFFSET_VDMA_S2MM_FRMDLY_STRIDE          0xa8
#define OFFSET_VDMA_S2MM_FRAMEBUFFER1           0xac
#define OFFSET_VDMA_S2MM_FRAMEBUFFER2           0xb0
#define OFFSET_VDMA_S2MM_FRAMEBUFFER3           0xb4
#define OFFSET_VDMA_S2MM_FRAMEBUFFER4           0xb8

/* S2MM and MM2S control register flags */
#define VDMA_CONTROL_REGISTER_START                     0x00000001
#define VDMA_CONTROL_REGISTER_CIRCULAR_PARK             0x00000002
#define VDMA_CONTROL_REGISTER_RESET                     0x00000004
#define VDMA_CONTROL_REGISTER_GENLOCK_ENABLE            0x00000008
#define VDMA_CONTROL_REGISTER_FrameCntEn                0x00000010
#define VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK          0x00000080
#define VDMA_CONTROL_REGISTER_WrPntr                    0x00000f00
#define VDMA_CONTROL_REGISTER_FrmCtn_IrqEn              0x00001000
#define VDMA_CONTROL_REGISTER_DlyCnt_IrqEn              0x00002000
#define VDMA_CONTROL_REGISTER_ERR_IrqEn                 0x00004000
#define VDMA_CONTROL_REGISTER_Repeat_En                 0x00008000
#define VDMA_CONTROL_REGISTER_InterruptFrameCount       0x00ff0000
#define VDMA_CONTROL_REGISTER_IRQDelayCount             0xff000000

/* S2MM status register */
#define VDMA_STATUS_REGISTER_HALTED                     0x00000001  // Read-only
#define VDMA_STATUS_REGISTER_VDMAInternalError          0x00000010  // Read or write-clear
#define VDMA_STATUS_REGISTER_VDMASlaveError             0x00000020  // Read-only
#define VDMA_STATUS_REGISTER_VDMADecodeError            0x00000040  // Read-only
#define VDMA_STATUS_REGISTER_StartOfFrameEarlyError     0x00000080  // Read-only
#define VDMA_STATUS_REGISTER_EndOfLineEarlyError        0x00000100  // Read-only
#define VDMA_STATUS_REGISTER_StartOfFrameLateError      0x00000800  // Read-only
#define VDMA_STATUS_REGISTER_FrameCountInterrupt        0x00001000  // Read-only
#define VDMA_STATUS_REGISTER_DelayCountInterrupt        0x00002000  // Read-only
#define VDMA_STATUS_REGISTER_ErrorInterrupt             0x00004000  // Read-only
#define VDMA_STATUS_REGISTER_EndOfLineLateError         0x00008000  // Read-only
#define VDMA_STATUS_REGISTER_FrameCount                 0x00ff0000  // Read-only
#define VDMA_STATUS_REGISTER_DelayCount                 0xff000000  // Read-only

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define UIO_MAX_NAME_SIZE	64
#define UIO_MAX_NUM		255

#define UIO_INVALID_SIZE	-1
#define UIO_INVALID_ADDR	(~0)

#define UIO_MMAP_NOT_DONE	0
#define UIO_MMAP_OK		1
#define UIO_MMAP_FAILED		2
#define MAX_UIO_MAPS 	5

typedef struct {
    unsigned int baseAddr;
    int vdmaHandler;
    int width;
    int height;
    int pixelLength;
    int fbLength;
    unsigned int* vdmaVirtualAddress;
    unsigned int* fb1VirtualAddress;
    unsigned int* fb1PhysicalAddress;
    unsigned int* fb2VirtualAddress;
    unsigned int* fb2PhysicalAddress;
    unsigned int* fb3VirtualAddress;
    unsigned int* fb3PhysicalAddress;

    pthread_mutex_t lock;
} vdma_handle;

vdma_handle handleGlobal;
uint8_t *mm_data_info;
int     staticX;

struct uio_map_t {
	unsigned long addr;
	int size;
	int mmap_result;
};

struct uio_dev_attr_t {
	char name[ UIO_MAX_NAME_SIZE ];
	char value[ UIO_MAX_NAME_SIZE ];
	struct uio_dev_attr_t *next;
};

struct uio_info_t {
	int uio_num;
	struct uio_map_t maps[ MAX_UIO_MAPS ];
	unsigned long event_count;
	char name[ UIO_MAX_NAME_SIZE ];
	char version[ UIO_MAX_NAME_SIZE ];
	struct uio_dev_attr_t *dev_attrs;
	struct uio_info_t* next;  /* for linked list */
};

void uio_single_mmap_test(struct uio_info_t* info, int map_num){
	info->maps[map_num].mmap_result = UIO_MMAP_NOT_DONE;
	if (info->maps[map_num].size <= 0)
		return;
	info->maps[map_num].mmap_result = UIO_MMAP_FAILED;
	char dev_name[16];
	sprintf(dev_name,"/dev/uio%d",info->uio_num);
	int fd = open(dev_name,O_RDONLY);
	if (fd < 0)
		return;
	void* map_addr = mmap( NULL,
			       info->maps[map_num].size,
			       PROT_READ,
			       MAP_SHARED,
			       fd,
			       map_num*getpagesize());
	if (map_addr != MAP_FAILED) {
		info->maps[map_num].mmap_result = UIO_MMAP_OK;
		munmap(map_addr, info->maps[map_num].size);
	}
	close(fd);
}

void uio_mmap_test(struct uio_info_t* info){
	int map_num;
	for (map_num= 0; map_num < MAX_UIO_MAPS; map_num++)
		uio_single_mmap_test(info, map_num);
} 

static void show_device(struct uio_info_t *info){
	char dev_name[16];
	sprintf(dev_name,"uio%d",info->uio_num);
	printf("%s: name=%s, version=%s, events=%d\n",
	       dev_name, info->name, info->version, info->event_count);
}

static int show_map(struct uio_info_t *info, int map_num){
	if (info->maps[map_num].size <= 0)
		return -1;

	printf("\tmap[%d]: addr=0x%08X, size=%d",
	       map_num,
	       info->maps[map_num].addr,
	       info->maps[map_num].size);

	//if (opt_mmap) {
		printf(", mmap test: ");
		switch (info->maps[map_num].mmap_result) {
			case UIO_MMAP_NOT_DONE:
				printf("N/A");
				break;
			case UIO_MMAP_OK:
				printf("OK");
				break;
			default:
				printf("FAILED");
		}
	//}
	printf("\n");
	return 0;
}

static void show_maps(struct uio_info_t *info){
	int ret;
	int mi = 0;
	do {
		ret = show_map(info, mi);
		mi++;
	} while ((ret == 0)&&(mi < MAX_UIO_MAPS));
}

void uio_free_dev_attrs(struct uio_info_t* info){
	struct uio_dev_attr_t *p1, *p2;
	p1 = info->dev_attrs;
	while (p1) {
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	info->dev_attrs = NULL;
}

void uio_free_info(struct uio_info_t* info){
	struct uio_info_t *p1,*p2;
	p1 = info;
	while (p1) {
		uio_free_dev_attrs(p1);
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
}

int uio_get_mem_size(struct uio_info_t* info, int map_num){
	int ret;
	char filename[64];
	info->maps[map_num].size = UIO_INVALID_SIZE;
	sprintf(filename, "/sys/class/uio/uio%d/maps/map%d/size",
		info->uio_num, map_num);
	FILE* file = fopen(filename,"r");
	if (!file) return -1;
	ret = fscanf(file,"0x%lx",&info->maps[map_num].size);
	fclose(file);
	if (ret<0) return -2;
	return 0;
}

int uio_get_mem_addr(struct uio_info_t* info, int map_num){
	int ret;
	char filename[64];
	info->maps[map_num].addr = UIO_INVALID_ADDR;
	sprintf(filename, "/sys/class/uio/uio%d/maps/map%d/addr",
		info->uio_num, map_num);
	FILE* file = fopen(filename,"r");
	if (!file) return -1;
	ret = fscanf(file,"0x%lx",&info->maps[map_num].addr);
	fclose(file);
	if (ret<0) return -2;
	return 0;
}

int uio_get_event_count(struct uio_info_t* info){
	int ret;
	char filename[64];
	info->event_count = 0;
	sprintf(filename, "/sys/class/uio/uio%d/event", info->uio_num);
	FILE* file = fopen(filename,"r");
	if (!file) return -1;
	ret = fscanf(file,"%d",&info->event_count);
	fclose(file);
	if (ret<0) return -2;
	return 0;
}

int line_from_file(char *filename, char *linebuf){
	char *s;
	int i;
	memset(linebuf, 0, UIO_MAX_NAME_SIZE);
	FILE* file = fopen(filename,"r");
	if (!file) return -1;
	s = fgets(linebuf,UIO_MAX_NAME_SIZE,file);
	if (!s) return -2;
	for (i=0; (*s)&&(i<UIO_MAX_NAME_SIZE); i++) {
		if (*s == '\n') *s = 0;
		s++;
	}
	return 0;
}

int uio_get_name(struct uio_info_t* info){
	char filename[64];
	sprintf(filename, "/sys/class/uio/uio%d/name", info->uio_num);

	return line_from_file(filename, info->name);
}

int uio_get_version(struct uio_info_t* info){
	char filename[64];
	sprintf(filename, "/sys/class/uio/uio%d/version", info->uio_num);

	return line_from_file(filename, info->version);
}

int uio_get_all_info(struct uio_info_t* info){
	int i;
	if (!info)
		return -1;
	if ((info->uio_num < 0)||(info->uio_num > UIO_MAX_NUM))
		return -1;
	for (i = 0; i < MAX_UIO_MAPS; i++) {
		uio_get_mem_size(info, i);
		uio_get_mem_addr(info, i);
	}
	uio_get_event_count(info);
	uio_get_name(info);
	uio_get_version(info);
	return 0;
}

int uio_num_from_filename(char* name){
	enum scan_states { ss_u, ss_i, ss_o, ss_num, ss_err };
	enum scan_states state = ss_u;
	int i=0, num = -1;
	char ch = name[0];
	while (ch && (state != ss_err)) {
		switch (ch) {
			case 'u': if (state == ss_u) state = ss_i;
				  else state = ss_err;
				  break;
			case 'i': if (state == ss_i) state = ss_o;
				  else state = ss_err;
				  break;
			case 'o': if (state == ss_o) state = ss_num;
				  else state = ss_err;
				  break;
			default:  if (  (ch>='0') && (ch<='9')
				      &&(state == ss_num) ) {
					if (num < 0) num = (ch - '0');
					else num = (num * 10) + (ch - '0');
				  }
				  else state = ss_err;
		}
		i++;
		ch = name[i];
	}
	if (state == ss_err) num = -1;
	return num;
}

static struct uio_info_t* info_from_name(char* name, int filter_num){
	struct uio_info_t* info;
	int num = uio_num_from_filename(name);
	if (num < 0)
		return NULL;
	if ((filter_num >= 0) && (num != filter_num))
		return NULL;

	info = malloc(sizeof(struct uio_info_t));
	if (!info)
		return NULL;
	memset(info,0,sizeof(struct uio_info_t));
	info->uio_num = num;

	return info;
}

struct uio_info_t* uio_find_devices(int filter_num, int *uioCount){
	struct dirent **namelist;
	struct uio_info_t *infolist = NULL, *infp, *last;
	int n;

	n = scandir("/sys/class/uio", &namelist, 0, alphasort);

    // this is because the . and .. appear as names in the directory
	*uioCount = n - 2;

	if (n < 0)
		return NULL;

	while(n--) {
		infp = info_from_name(namelist[n]->d_name, filter_num);
		free(namelist[n]);
		if (!infp)
			continue;
		if (!infolist)
			infolist = infp;
		else
			last->next = infp;
		last = infp;
	}
	free(namelist);

	return infolist;
}
/*
void vdma_halt(vdma_handle *handle) {
	printf("halting\r\n");
    vdma_set(handle, OFFSET_VDMA_S2MM_CONTROL_REGISTER, VDMA_CONTROL_REGISTER_RESET);
    vdma_set(handle, OFFSET_VDMA_MM2S_CONTROL_REGISTER, VDMA_CONTROL_REGISTER_RESET);
    munmap((void *)handle->vdmaVirtualAddress, 65535);
    munmap((void *)handle->fb1VirtualAddress, handle->fbLength);
    munmap((void *)handle->fb2VirtualAddress, handle->fbLength);
    munmap((void *)handle->fb3VirtualAddress, handle->fbLength);
    close(handle->vdmaHandler);
}
*/

void vdma_halt(vdma_handle *handle) {
	printf("halting\r\n");
    vdma_set(handle, OFFSET_VDMA_S2MM_CONTROL_REGISTER, VDMA_CONTROL_REGISTER_RESET);
    vdma_set(handle, OFFSET_VDMA_MM2S_CONTROL_REGISTER, VDMA_CONTROL_REGISTER_RESET);
    munmap((void *)handle->vdmaVirtualAddress, 65535);
    munmap((void *)handle->fb1VirtualAddress, handle->fbLength*3);
    close(handle->vdmaHandler);
}

unsigned int vdma_get(vdma_handle *handle, int num) {
    return handle->vdmaVirtualAddress[num>>2];
}

void vdma_set(vdma_handle *handle, int num, unsigned int val) {
    handle->vdmaVirtualAddress[num>>2]=val;
}

int vdma_setup(vdma_handle *handle, unsigned int baseAddr, int width, int height, int pixelLength, unsigned int fb1Addr, unsigned int fb2Addr, unsigned int fb3Addr) {
    handle->baseAddr=baseAddr;
    handle->width=width;
    handle->height=height;
    handle->pixelLength=pixelLength;
    handle->fbLength=pixelLength*width*height;
    handle->vdmaHandler = open("/dev/mem", O_RDWR | O_SYNC);
    handle->vdmaVirtualAddress = (unsigned int*)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, handle->vdmaHandler, (off_t)handle->baseAddr);
    if(handle->vdmaVirtualAddress == MAP_FAILED) {
        perror("vdmaVirtualAddress mapping for absolute memory access failed.\n");
        return -1;
    }

    handle->fb1PhysicalAddress = fb1Addr;
    handle->fb1VirtualAddress = (unsigned char*)mmap(NULL, handle->fbLength, PROT_READ | PROT_WRITE, MAP_SHARED, handle->vdmaHandler, (off_t)fb1Addr);
    if(handle->fb1VirtualAddress == MAP_FAILED) {
        perror("fb1VirtualAddress mapping for absolute memory access failed.\n");
        return -2;
    }

    handle->fb2PhysicalAddress = fb2Addr;
    handle->fb2VirtualAddress = (unsigned char*)mmap(NULL, handle->fbLength, PROT_READ | PROT_WRITE, MAP_SHARED, handle->vdmaHandler, (off_t)fb2Addr);
    if(handle->fb2VirtualAddress == MAP_FAILED) {
        perror("fb2VirtualAddress mapping for absolute memory access failed.\n");
        return -3;
    }

    handle->fb3PhysicalAddress = fb3Addr;
    handle->fb3VirtualAddress = (unsigned char*)mmap(NULL, handle->fbLength, PROT_READ | PROT_WRITE, MAP_SHARED, handle->vdmaHandler, (off_t)fb3Addr);
    if(handle->fb3VirtualAddress == MAP_FAILED)
    {
     perror("fb3VirtualAddress mapping for absolute memory access failed.\n");
     return -3;
    }

    return 0;
}

int vdma_setup2(vdma_handle *handle, int width, int height, int pixelLength, char *name0, char *name1) {
	struct uio_info_t *info_list, *p, *regs, *bufs;
	int compareResult;
	int uioNum = -1;
	int uioCount = 0;
	char dev_name[16];
	int fd1;
	int fd2;
	int fd3;
	int fd4;
	int map_num = 0;
	
	printf("in vdma setup2!!\r\n");
	
	info_list = uio_find_devices(-1,&uioCount);
	if (!info_list)
		printf("No UIO devices found.\n");

	p = info_list;

	while (p) {
		uio_get_all_info(p);

		//compareResult = strcmp("vdma1-read-reg",p->name);
		compareResult = strcmp(name0,p->name);
		if (compareResult == 0){
			regs = p;
		}
		
		//compareResult = strcmp("vdma1-read-buf1",p->name);
		compareResult = strcmp(name1,p->name);
		if (compareResult == 0){
			bufs = p;
		}
		
		//compareResult = strcmp("vdma1-read-buf2",p->name);
		//compareResult = strcmp(name2,p->name);
		//if (compareResult == 0){
		//	buf2 = p;
		//}
		
		//compareResult = strcmp("vdma1-read-buf3",p->name);
		//compareResult = strcmp(name3,p->name);
		//if (compareResult == 0){
		//	buf3 = p;
		//}
		
		p = p->next;
	}

    //handle->baseAddr=baseAddr;
    handle->width=width;
    handle->height=height;
    handle->pixelLength=pixelLength;
    handle->fbLength=pixelLength*width*height;
	
	// setup vdma axi lite mapping
	sprintf(dev_name,"/dev/uio%d",regs->uio_num);
	fd1 = open(dev_name,O_RDWR | O_SYNC);

	handle->vdmaVirtualAddress = mmap( NULL,
			       regs->maps[map_num].size,
			       PROT_READ | PROT_WRITE,
			       MAP_SHARED,
			       fd1,
			       map_num*getpagesize());
				   
	// setup frame buffer 1
	sprintf(dev_name,"/dev/uio%d",bufs->uio_num);
	fd2 = open(dev_name,O_RDWR | O_SYNC);
	
	map_num = 0;
	handle->fb1PhysicalAddress = bufs->maps[map_num].addr;
	handle->fb1VirtualAddress = mmap( NULL,
			       bufs->maps[map_num].size,
			       PROT_READ | PROT_WRITE,
			       MAP_SHARED,
			       fd2,
			       map_num*getpagesize());
	
	// setup frame buffer 2
	//sprintf(dev_name,"/dev/ui	o%d",buf2->uio_num);
	//fd3 = open(dev_name,O_RDWR | O_SYNC);

	handle->fb2PhysicalAddress = handle->fb1PhysicalAddress + 0x400000;

	handle->fb2VirtualAddress = handle->fb1VirtualAddress + 0x400000;

	//handle->fb2VirtualAddress = mmap( NULL,
	//		       buf2->maps[map_num].size,
	//		       PROT_READ | PROT_WRITE,
	//		       MAP_SHARED,
	//		       fd3,
	//		       map_num*getpagesize());

	// setup frame buffer 3
	//sprintf(dev_name,"/dev/uio%d",buf3->uio_num);
	//fd4 = open(dev_name,O_RDWR | O_SYNC);

	handle->fb3PhysicalAddress = handle->fb2PhysicalAddress + 0x400000;
	handle->fb3VirtualAddress = handle->fb2VirtualAddress + 0x400000;

	//handle->fb3VirtualAddress = mmap( NULL,
	//		       buf3->maps[map_num].size,
	//		       PROT_READ | PROT_WRITE,
	//		       MAP_SHARED,
	//		       fd4,
	//		       map_num*getpagesize());
    
    return 0;
}

void vdma_start_triple_buffering(vdma_handle *handle) {
    // Reset VDMA
    vdma_set(handle, OFFSET_VDMA_S2MM_CONTROL_REGISTER, VDMA_CONTROL_REGISTER_RESET);

    // Wait for reset to finish
    while((vdma_get(handle, OFFSET_VDMA_S2MM_CONTROL_REGISTER) & VDMA_CONTROL_REGISTER_RESET)==4);

    // Clear all error bits in status register
    vdma_set(handle, OFFSET_VDMA_S2MM_STATUS_REGISTER, 0);

    // Do not mask interrupts
    vdma_set(handle, OFFSET_VDMA_S2MM_IRQ_MASK, 0xf);

    int interrupt_frame_count = 3;

    // Start both S2MM and MM2S in triple buffering mode
    vdma_set(handle, OFFSET_VDMA_S2MM_CONTROL_REGISTER,
        (interrupt_frame_count << 16) |
        VDMA_CONTROL_REGISTER_START |
        VDMA_CONTROL_REGISTER_GENLOCK_ENABLE |
        VDMA_CONTROL_REGISTER_INTERNAL_GENLOCK |
        VDMA_CONTROL_REGISTER_CIRCULAR_PARK);
        
    while((vdma_get(handle, 0x30)&1)==0 || (vdma_get(handle, 0x34)&1)==1) {
        printf("Waiting for VDMA to start running...\n");
        sleep(1);
    }

    // Extra register index, use first 16 frame pointer registers
    vdma_set(handle, OFFSET_VDMA_S2MM_REG_INDEX, 0);

    // Write physical addresses to control register
    vdma_set(handle, OFFSET_VDMA_S2MM_FRAMEBUFFER1, handle->fb1PhysicalAddress);
    vdma_set(handle, OFFSET_VDMA_S2MM_FRAMEBUFFER2, handle->fb2PhysicalAddress);
    vdma_set(handle, OFFSET_VDMA_S2MM_FRAMEBUFFER3, handle->fb3PhysicalAddress);

    // Write Park pointer register
    vdma_set(handle, OFFSET_PARK_PTR_REG, 0);

    // Frame delay and stride (bytes)
    vdma_set(handle, OFFSET_VDMA_S2MM_FRMDLY_STRIDE, handle->width*handle->pixelLength);

    // Write horizontal size (bytes)
    vdma_set(handle, OFFSET_VDMA_S2MM_HSIZE, handle->width*handle->pixelLength);

    // Write vertical size (lines), this actually starts the transfer
    vdma_set(handle, OFFSET_VDMA_S2MM_VSIZE, handle->height);
}

int init(char *name0, char *name1, int width, int height, int depth){
  int cached;
  int fd;
  vdma_setup(&handleGlobal, 0x43000000, 752, 480, 8, 0x31000000, 0x32000000, 0x33000000);
  //vdma_setup2(&handleGlobal,width,height,depth,name0,name1);
  vdma_start_triple_buffering(&handleGlobal);
  cached = 0;
  fd = open("/dev/mem", O_RDWR|(!cached ? O_SYNC : 0));
  mm_data_info = mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x43C50000);
  staticX = 0;
  return(0);
}

int getFrame(void * indatav){
  uint32_t* indata;
  indata = (uint32_t*) indatav;
  int x;
  x = (*(volatile int *)(mm_data_info + 12));
  if (x != staticX) {   
    if ((x == 1) || (x == 6)){
        memcpy((indata), handleGlobal.fb3VirtualAddress, MAP_SIZE); 
    }
    else if ((x == 3) || (x == 7)){
        memcpy((indata), handleGlobal.fb1VirtualAddress, MAP_SIZE); 
    }
    else if ((x == 2) || (x == 5)){
        memcpy((indata), handleGlobal.fb2VirtualAddress, MAP_SIZE); 
    }
    else{
        printf("error in imageFeedthroughDriver.so\n");
    } 
    staticX = x;
    return(0);
  }
  else{
    return(1);
  }
}

int destroy(){
  munmap((void *)mm_data_info, 32);
  vdma_halt(&handleGlobal);
  return(0);
}