/*
 * File: hal.h
 *
 *  HAL header file.
 */

 
 
#define PARTITION_BOOT_FLAG 0x80
//#define PARTITION_ 


//#define BOOT_RECORD_SIGNATURE          (0xaa55)



/*
 * Microcontrollers support. @todo: Criar arquivo para isso.
 */

 
//#define Microcontroller_PIC 1 
//#define Microcontroller_ 
//...


  
/*
 * vari�veis usadas no hal.
 *
 */  
  
unsigned long g_machine_type;  
  
//
//....
//

//Hardware:
//Estrutura para todos os componentes de hardware presentes.
typedef struct hardware_d hardware_t;
struct hardware_d
{
	//
	// MainBoard info.
	//
	struct motherboard_d *MotherBoard;
	
	//
	// Processor info.
	//
	struct tagProcessor *Processor;   //@todo: mudar para processo_d
	int ProcessorArchitecture;
	int NumberOfProcessors;
	
	
	//
	// Chipset info. (north /south)
	//
	
    //@todo: struct
	unsigned long NorthBridge;  // North Bridge (Device number) word
	unsigned long SouthBridge; 	// South Bridge (Device number) word

    //
    // RAM memory cards info.
    //
	struct memory_d *Memory;
	
	//
	// Video card info. (graphics)
	//
	//struct tagConsole
	//struct tagGraphics
	//struct d_video
	//struct screen_d 
	
	//
	// PCI and PCIEX slots info.
	//
	struct pci_d *Pci;
	
	//
	// PIC.
	//
	struct pic_d *Pic;
	
	//
	// PIT, Timer info.
	//
	//struct pit_d *Pit; @todo.
	struct timer_d *Timer;
	
	
	//
	// RTC CMOS info.
	//
	struct rtc_d *Rtc;
	
	//
	// Discos info.
	//
	struct diskinfo_d *Disk;
	
	//
	// Volumes dos discos.
	//
	struct volumeinfo_d *Volumes; //ponteiro para array de estruturas de volumes.
	
	
	//
	// Keyboard info.
	//
	struct keyboard_d *Keyboard;
	
	//
	// Mouse info.
	//
	
	
	//floppy fdd
	struct fdd_d *Fdd;
	//struct floppy_d *Floppy;
	
	
	//ide hdd
	struct hdd_d *Hdd;
	//struct ide_d *Ide;
	
	//devices
	struct devices_d *Devices; //array de estruturas
	
	//continua ...
};  
hardware_t *Hardware;  


//Firmware:
//Estrutura para todos os componentes de firmware presentes.
typedef struct firmware_d firmware_t;
struct firmware_d
{
	int dummy;
	//bios
	//...	
};  
firmware_t *Firmware;  


//
//....
//


  
//Estrutura de disco. @todo: Colocar em outro arquivo.  
typedef struct tagDriveContext DriveContext_t;
struct tagDriveContext 
{
    ULONG Drive;
    ULONG Cylinders;
    ULONG Heads;
    ULONG Sectors;
};
DriveContext_t *DriveContext;
 
  
//Estrutura para informa��es sobre a placa m�e.  
typedef struct motherboard_d motherboard_t; 
struct motherboard_d
{
    //...	
};  
//motherboard_t *MotherBoard;  


/*
 ******************************************************
 * ProcessorBlock_d:
 *     Processor Block.
 *     Usado para task switch.
 *     Cont�m informa��es sobre o processador. 
 *     Que processo est� nele.
 *     #importante:
 *     obs: talvez devamos ter um desses para cada 
 * processador ??
 *     +A contagem de thrads feita aqui refere-se somente 
 * a um processador ?
 *
 */
struct ProcessorBlock_d
{
    object_type_t objectType;
	object_class_t objectClass;	
	
    //
	// Processor Info. 
	//
	
	int Id;
	int Used;
	struct tagProcessor *processorInfo; //informa��es sobre o processador. 
	
	//
	// Process.
	//
	
	//int running_processes;
	int processes_counter;
	
	struct process_d *CurrentProcess;  
	struct process_d *KernelProcess;  
	//struct process_d *CurrentProcess;  
	
	//
	// Threads.
	//

	//N�mero de threads rodando nesse processador.
	//n�o importa o estado que elas estejam, ent�o 
	//talvez esse nome n�o seja apropriado, pois 
	//d� a impress�o que a trhead est� no rodando no momento.
	
	//int running_threads;   
    int threads_counter;
	
	struct thread_d *CurrentThread;    
    struct thread_d *NextThread;
    struct thread_d *IdleThread;	
	//...	
	
    //Continua ...
};

//Obs: N�o tem ponteiro.
//Pois se refere ao processador atual, quando temos apenas 
//um processador.
struct ProcessorBlock_d ProcessorBlock;    
  

//lista de informa��es sobre os processadores.  
//unsigned long processorblocList[32];  

  
//...
 



//
// GUI support.
// 
 
void 
hal_backbuffer_putpixel ( unsigned long ax, 
                          unsigned long bx, 
						  unsigned long cx, 
						  unsigned long dx ); 
							  
void 
hal_lfb_putpixel ( unsigned long ax, 
                   unsigned long bx, 
				   unsigned long cx, 
				   unsigned long dx ); 
							  
							  
							  
							
		 
				 
/*
VOID
NSDumpMemory(
    PVOID Start,
    ULONG Length
    )
{
    ULONG cnt;

    BlPrint(" %lx:\n",(ULONG)Start);
    for (cnt=0; cnt<Length; cnt++) {
        BlPrint("%x ",*((PUSHORT)(Start)+cnt));
        if (((cnt+1)%16)==0) {
            BlPrint("\n");
        }
    }
}
*/
				
/*
 * @todo: unsigned long bios_call(unsigned long a, b c d edi esi ebp);
 */
 
/* @todo: partition. 
NTHALAPI
NTSTATUS
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTHALAPI
NTSTATUS
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    ); 
*/				
 
//Initialization support. 
int init_hal();


int init_amd();


int jmp_address( unsigned long arg1, 
                 unsigned long arg2, 
				 unsigned long arg3 , 
				 unsigned long arg4); 
				 





int hal_init_machine(); 
void hal_set_machine_type(unsigned long type);
unsigned long hal_get_machine_type();	
int hal_hardware_detect();	
int hal_showpciinfo();		
void hal_vsync();
void hal_reboot();
void hal_shutdown();

//
// 
//

unsigned long getGdt();
unsigned long getIdt();
//unsigned long getTss();  //todo


//
//  VECTORS (isr and irq address)
//



// Endere�os das rotinas b�sicas chamadas pelos vetores de interrup��es.
// Essas rotinas devem chamar seus handlers espec�ficos.
// 256 interrup��es
unsigned long VECTORS[256];



void hal_setup_new_vectors_table_entry ( int number, unsigned long address );

// vetores legados.
//Inicializando a tabela de vetores com os endere�os das rotinas usadas pelo assembler
//na inicializa��o de alguns vetores de interrup��o.
void hal_init_vectors_table();



//
// HANDLERS - (callbacks) 
//


// Endere�os para as rotinas em C que tratam as interrup��es.
// Essas rotinas rodam depois da rotina b�sica em assembly.
// 256 interrup��es
// 8 extras para handlers default.
unsigned long HANDLERS[256+8];

void hal_idt_register_interrupt ( unsigned long idt_location, unsigned char i, unsigned long callback );
void hal_default_handler();
void hal_init_handlers_table();
void hal_setup_new_handler ( int number, unsigned long callback );
void hal_invalidate_handler (int number);



//
// End.
//

