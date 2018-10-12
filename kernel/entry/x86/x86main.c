/*
 * Gramado Operating System - The main file for the kernel.
 * (c) Copyright 2015~2018 - Fred Nora.
 *
 * File: k\entry\x86\x86main.c 
 * 
 * Description:
 *     This is the Kernel Base. 
 *     It's the mains file for a 32 bit x86 Kernel. 
 *     The type is 'hybrid'.
 *     The entry point is in 'head.s'.
 *
 * This is classes for a logic hardware layout.
 *
 * **** Classes: ****
 * ==================
 * 1) kernel.ram               (K5)
 * 2) kernel.io.cpu            (K4)
 * 2) kernel.io.dma            (K3)
 * 3) kernel.devices.unbloqued (K2)
 * 3) kernel.devices.blocked   (K1)
 * 4) kernel.things            (K0)
 *
 * The first three system utilities are: 
 * IDLE.BIN, SHELL.BIN and TASKMAN.BIN.
 *
 * The Kernel area is the first 4MB of real memory.
 * The image was loaded in the address 0x00100000 and the entry point is in 
 * the address 0x00101000. The logic address for the Kernel image is 
 * 0xC0001000 and the entry point is 0xC0001000.
 * 
 * @todo: Create a log file.
 *
 *  In this file:
 *  ============
 *      + mainSetCr3
 *      + startStartIdle
 *      + x86main - The entry point for a C part of the Kernel.
 *
 * Revision History:
 *     2015      - Created by Fred Nora.
 *     2016~2018 - Revision.
 *     //... 
 */ 
#include <kernel.h>


// # external dependencies #

// Variables from Boot Loader.
extern unsigned long SavedBootBlock;    //Boot Loader Block.
extern unsigned long SavedLFB;          //LFB address.  
extern unsigned long SavedX;            //Screen width. 
extern unsigned long SavedY;            //Screen height.
extern unsigned long SavedBPP;          //Bits per pixel.
//...

// Args.
extern unsigned long kArg1;
extern unsigned long kArg2;
extern unsigned long kArg3;
extern unsigned long kArg4;
//...

// Boot mode.
extern unsigned long SavedBootMode;


// Task switching support.
extern void turn_task_switch_on();


char copyright[] =
"Copyright (c) 2005-2018\n\tFred Nora.  All rights reserved.\n\n";


static inline void mainSetCr3 ( unsigned long value ) {
    __asm__ ( "mov %0, %%cr3" : : "r"(value) );
};


/*
 *********************************************************
 * startStartIdle:
 *     Initializes idle thread.
 *     # fake idle thread 
 *     idle thread em ring3.
 *
 *     @todo: 
 *         + Initializes idle thread.
 *           startIdleThread().
 *
 */
void startStartIdle (){
	
    int i;
 
    if ( (void *) IdleThread == NULL )
    {
        printf("main-startStartIdle: IdleThread\n");
        die();
    }else{

        if(IdleThread->saved != 0){
            printf("main-startStartIdle: saved\n");
            die();
        };

        if(IdleThread->used != 1 || IdleThread->magic != 1234){
            printf("main-startStartIdle: tid={%d} magic \n", IdleThread->tid);
            die();
        };

        set_current(IdleThread->tid);       
        //...
    };

    // State  
    if ( IdleThread->state != STANDBY ){
        printf("main-startStartIdle: state tid={%d}\n",IdleThread->tid);
        die();
    }

    // * MOVEMENT 2 ( Standby --> Running)
    if ( IdleThread->state == STANDBY ){
        IdleThread->state = RUNNING;
        queue_insert_data(queue, (unsigned long) IdleThread, QUEUE_RUNNING);
    }


	//Current process.
	current_process = IdleThread->process->pid;
	
//Done!
//done:
    //Debug:
 
    //printf("* Starting idle TID=%d \n", Thread->tid);
    //refresh_screen(); //@todo:  


    for ( i=0; i <= DISPATCHER_PRIORITY_MAX; i++ ){
        dispatcherReadyList[i] = (unsigned long) IdleThread;
    }


    IncrementDispatcherCount(SELECT_IDLE_COUNT);


    //Set cr3 and flush TLB.
    mainSetCr3( (unsigned long) IdleThread->Directory);
    asm("movl %cr3, %eax");
    asm("movl %eax, %cr3");


    /* turn_task_switch_on:
     * + Creates a vector for timer irq, IRQ0.
     * + Enable taskswitch. */
	 
    turn_task_switch_on();

    timerInit8253();
	
	//parece que isso é realmente preciso, libera o teclado.
	//outb(0x20,0x20); 
   
	// # go!
	// Nos configuramos a idle thread em user mode e agora vamos saltar 
	// para ela via iret.
	
    asm volatile(" cli \n"
                 " mov $0x23, %ax  \n"
                 " mov %ax, %ds  \n"
                 " mov %ax, %es  \n"
                 " mov %ax, %fs  \n"
                 " mov %ax, %gs  \n"
                 " pushl $0x23  \n"              // ss.
                 " movl $0x0044FFF0, %eax  \n"
                 " pushl %eax  \n"               // esp.
                 " pushl $0x3200  \n"            // eflags.
                 " pushl $0x1B  \n"              // cs.
                 " pushl $0x00401000  \n"        // eip.
                 " iret \n" );

    panic("main-startStartIdle:");
};



/*
 *************************************************
 * kMain: 
 *     The entry point for a C part of the Kernel.
 *
 * Function history:
 *     2015 - Created by Fred Nora.
 *     2016~2018 - Revision.
 *     ...
 */
int x86main ( int argc, char *argv[] ){
	
    int Status = 0;
    int zIndex;
	
    KernelStatus = KERNEL_NULL;

    //Initializing the global spinlock.
    __ipc_kernel_spinlock = 1;

    // #test.
    // initializing zorder list.
    
    for ( zIndex = 0; zIndex < ZORDER_COUNT_MAX; zIndex++ ){
        zorderList[zIndex] = (unsigned long) 0;
    }

    //initializing zorder counter.
    zorderCounter = 0;

    // set system window procedure.
    SetProcedure( (unsigned long) &system_procedure);


    // Video.
    // First of all.
    // ps: Boot loader is mapping the LFB.

//setupVideo:

    // @todo: 
    // Device screen sizes.

    //Set graphics mode or text mode using a flag.
    if ( SavedBootMode == 1 ){
		
        g_useGUI = GUI_ON;
        VideoBlock.useGui = GUI_ON;
        //...
    }else{
        g_useGUI = GUI_OFF;
        VideoBlock.useGui = GUI_OFF;
        //...
    };
	
	
	//verbose mode do kernel.
    //initializes a new line when '\n' is found.
	stdio_verbosemode_flag = 1;

    videoVideo();
    videoInit();
	
    // Init screen

#ifdef KERNEL_VERBOSE	
    //If we are using graphics mode.
    if (VideoBlock.useGui == GUI_ON){
        printf("x86main: Using GUI\n");
    }
#endif

    //If we are using text mode.
    if (VideoBlock.useGui != GUI_ON)
	{
        set_up_text_color(0x0F, 0x00);

        //g_current_vm = 0x800000;
        //Set cga video memory: 0x800000vir = 0xb8000fis.
        videoSetupCGAStartAddress(SCREEN_CGA_START); 

    //Debug.
#ifdef KERNEL_VERBOSE
        kclear(0);
        kprint("x86main: Debug" ,9 ,9 );
        printf("x86main: Text Mode\n");
#endif
    };

#ifdef KERNEL_VERBOSE
    printf("x86main: Starting kernel..\n");
    refresh_screen(); 
#endif

//initializeSystem:

    systemSystem();	
    Status = (int) systemInit();
	
    if ( Status != 0 ){
        printf("x86main: systemInit\n");
        KernelStatus = KERNEL_ABORTED;
        goto fail;
    }
	
	
	//  ## Processes ##

    //=================================================
    // processes and threads initialization.
    // Creating processes and threads.
    // The processes are: Kernel, Idle, Shell, Taskman.
    // ps: The images are loaded in the memory.

//createProcesses:

    // Creating Kernel process. PID=0.
    KernelProcess = (void *) create_process( NULL, // Window station.
	                                         NULL, // Desktop.
											 NULL, // Window.
											 (unsigned long) 0xC0001000,  // Entry point. 
                                             PRIORITY_HIGH,               // Priority.
											 (int) 0,                     // ppid.
											 "KERNEL-PROCESS",            // Name.
											 RING0,                       // iopl. 
											 (unsigned long ) KERNEL_PAGEDIRECTORY ); // Page directory.	
    if( (void *) KernelProcess == NULL )
	{
        printf("x86main: KernelProcess\n");
        die();
    }else{
 
        processor->CurrentProcess = (void *) KernelProcess;
        processor->NextProcess = (void *) KernelProcess;
        //...
    };


    //Creating Idle process.
    InitProcess = (void *) create_process( NULL, 
	                                      NULL, 
										  NULL, 
										  (unsigned long) 0x00401000, 
                                          PRIORITY_HIGH, 
										  (int) KernelProcess->pid, 
										  "IDLEPROCESS", 
										  RING3, 
										  (unsigned long ) KERNEL_PAGEDIRECTORY );	
    if ( (void *) InitProcess == NULL )
	{
        printf("x86main: InitProcess\n");
        die();
    }else{
        //processor->IdleProcess = (void*) IdleProcess;	
    };

    //Creating Shell process.
    ShellProcess = (void *) create_process( NULL, 
	                                       NULL, 
										   NULL, 
										   (unsigned long) 0x00401000, 
                                           PRIORITY_HIGH, 
										   (int) KernelProcess->pid, 
										   "SHELLPROCESS", 
										   RING3, 
										   (unsigned long ) KERNEL_PAGEDIRECTORY );	
    if((void *) ShellProcess == NULL){
        printf("x86main: ShellProcess\n");
        die();
    }else{
        //...
    };
	
	
	
    //Creating Taskman process. 
    TaskManProcess = (void *) create_process( NULL, 
	                                         NULL, 
											 NULL, 
											 (unsigned long) 0x00401000, 
                                             PRIORITY_LOW, 
											 KernelProcess->pid, 
											 "TASKMANPROCESS", 
											 RING3, 
											 (unsigned long ) KERNEL_PAGEDIRECTORY );	
    if ( (void *) TaskManProcess == NULL ){
        printf("x86main: TaskManProcess\n");
        die();
    }else{
        //...
    };
	
	
	//  ## Threads  ##
	
    // Creating threads. 
    // The threads are: Idle, Shell, Taskman.
    // The Idle thread belong to Idle process.
    // The Shell thread belongs to Shell process.
    // The Taskman thread belongs to Taskman process.

//createThreads:

    //====================================================
    //Create Idle Thread. tid=0. ppid=0.
    IdleThread = (void*) KiCreateIdle();
    if ( (void *) IdleThread == NULL )
	{
        printf("x86main: IdleThread\n");
        die();
    }else{

        IdleThread->ownerPID = (int) InitProcess->pid;

        //Thread.
        processor->CurrentThread = (void*) IdleThread;
        processor->NextThread    = (void*) IdleThread;
        processor->IdleThread    = (void*) IdleThread;
        //...
    };


    //=============================================
    // Create shell Thread. tid=1. 
    ShellThread = (void *) KiCreateShell();
    if( (void *) ShellThread == NULL )
	{
        printf("x86main: ShellThread\n");
        die();
    }else{

        ShellThread->ownerPID = (int) ShellProcess->pid;
        //...
    };

    //===================================
    //Create taskman Thread. tid=2.
    TaskManThread = (void *) KiCreateTaskManager();
    if( (void *) TaskManThread == NULL )
	{
        printf("x86main: TaskManThread\n");
        die();
    }else{

        TaskManThread->ownerPID = (int) TaskManProcess->pid;
        //...
    };

	
	
    //===================================
    // Cria uma thread em ring 0.
	// Ok. isso funcionou bem.
    RING0IDLEThread = (void *) KiCreateRing0Idle();
    if( (void *) RING0IDLEThread == NULL )
	{
        printf("x86main: RING0IDLEThread\n");
        die();
    }else{

        RING0IDLEThread->ownerPID = (int) TaskManProcess->pid;
        //...
    };
	
	
	//
	// ## importante ## 
	//
	
	next_thread = 0;
	
	// #importante: 
	// Essa não pode fechar nunca.
	// idle thread ... 
	idle = 3;
	
    //...


//Kernel base Debugger.
//doDebug:

    Status = (int) debug();
    
	if ( Status != 0 )
	{    
        printf("x86main: debug\n");
		KernelStatus = KERNEL_ABORTED;
        goto fail;
    }else{
        KernelStatus = KERNEL_INITIALIZED;
    };


    // TESTS:
    // We can make some tests here.


    //Inicializando as variáveis do cursor piscante do terminal.
    //isso é um teste.
    timer_cursor_used = 0;   //desabilitado.
    timer_cursor_status = 0;


//doTests:
   //...
	
    // Initializing ps/2 controller.
	//unblocked/ldisc.c
    ps2();
	
	
	//
	// IN:
	//    FORCEPIO = ATA driver operates in PIO mode.

    //ATAMSG_INITIALIZE = 1
    diskATADialog( 1, FORCEPIO, FORCEPIO );

    //
    // Loading file tests.
    //


   // void *b;


   /*
    //===================================
    // @todo: loads a bmp.
    //
    //janela de test
    CreateWindow( 1, 0, 0, "Fred-BMP-Window", 
                  (30-5), (450-5), (128+10), (128+10), 
                  gui->main, 0, COLOR_WINDOW, COLOR_WINDOW); 
    // testing BMP support 32KB
    b = (void*) malloc(512*1024);
    //#bugbug checar a validade do buffer

    unsigned long fileret;

    //taskswitch_lock();
    //scheduler_lock();
    fileret = fsLoadFile( "FREDNORABMP", (unsigned long) b);
    if(fileret != 0)
    {
        //escrevendo string na janela
        draw_text( gui->main, 10, 500, COLOR_WINDOWTEXT, "FREDNORA BMP FAIL");  	
    }
    bmpDisplayBMP( b, 30, 450, 128, 128 );
    //scheduler_unlock();
    //taskswitch_unlock();

    //===================================
    */

    /*
      * esse teste funcionou.
    //
    // testando a alocação de páginas.
    //

    printf("main: testing page allocator\n");

    // isso substitui o malloc.
    b = (void*) newPage();
    if( (void *) b == NULL ){
        printf("main: newPage: buffer:");
        refresh_screen();
        while(1){}
    }

    unsigned long fileret;
    fileret = fsLoadFile( "INIT    TXT", (unsigned long) b);
    if(fileret != 0)
    {
        //escrevendo string na janela
        draw_text( gui->main, 10, 500, COLOR_WINDOWTEXT, "INIT.TXT FAIL");
    }else{

        printf("%s",b);
        refresh_screen();
        while(1){}
    }

    //#debug
    printf("main: debug done");
    refresh_screen();
    while(1){}

    */
	
	/*
	
	// TESTANDO SALVAR UM ARQUIVO ...
    //?? stdin ??
	
	char file_1[] = "Isso é um arquivo de teste ..... \n testando salvar um arquivo!!!!:)";
	
	char file_name[] = "savetest.txt";
	write_fntos( (char *) file_name );
	

    //
    //  ## Save ##
    //
	
	fsSaveFile( file_name,  // Nome. 
	            3,          // Quantidade de setores.
				255,        // Tamanho do arquivo dado em bytes.     
				file_1,     // Buffer onde está o arquivo.
				0x20 );     // Tipo de entrada. 0x20=arquivo.
				
				
	*/
	
    
	//Isso funcionou, não mudar de lugar.
	//Mas isso faz parte da interface gráfica,
	//Só que somente nesse momento temos recursos 
	//suficientes para essa rotina funcionar.
	
	windowLoadGramadoIcons ();
	

	// ## Testando font nelson Cole 2 ##
    gwsInstallFont("NC2     FON");
	
	//
	// ## testando suporte a salvamento de retângulo ##
	//
	
	//#debug
	//printf ("testando salvamento de retangulo\n");
	//refresh_screen();
	
	initialize_saved_rect ();
	
	//testando salvar um retângulo ...
	//save_rect ( 0, 0, 100, 100 );
	
	//copiando aqui no backbuffer
	//show_saved_rect ( 20, 20, 100, 100 );
	
	//
	// ## servidor taskman ##
	//
	
	// ## Criando a janela do servidor taskman ## 
	// usada para comunicação.
	
	gui->taskmanWindow = (void *) CreateWindow( 1, 0, VIEW_MINIMIZED, "taskman-server-window", 
	                             1, 1, 1, 1,           
							     gui->main, 0, 0, COLOR_WINDOW  ); 
								 
	if ( (void *) gui->taskmanWindow == NULL )
	{
		printf("x86main: falaha ao criar a janela do servidor taskman\n");
		die();
	}else{
		
		//inicializando a primeira mensagem
		////envia uma mensagem de teste para o servidor taskman
	    gui->taskmanWindow->msg_window = NULL;
		gui->taskmanWindow->msg = 0; //temos uma mensagem.
		gui->taskmanWindow->long1 = 0;
		gui->taskmanWindow->long2 = 0;
        gui->taskmanWindow->newmessageFlag = 0;		
		
	    //gui->taskmanWindow->msg_window = NULL;
		//gui->taskmanWindow->msg = 123; //temos uma mensagem.
		//gui->taskmanWindow->long1 = 0;
		//gui->taskmanWindow->long2 = 0;
        //gui->taskmanWindow->newmessageFlag = 1;		
		
	};
	
	//#debug
	//extern unsigned long code_begin;
	//extern unsigned long code_end;
	//extern unsigned long data_begin;
	//extern unsigned long data_end;
	//extern unsigned long bss_begin;
	//extern unsigned long bss_end;
	
	//printf("\n");
	//printf("\n");
	//printf("#debug\n");
	//printf("======\n");
	
	//printf("\n");
	//printf("code_begin={%x}\n", &code_begin );
	//printf("  code_end={%x}\n", &code_end );

	//printf("\n");
	//printf("data_begin={%x}\n", &data_begin );
	//printf("  data_end={%x}\n", &data_end );

	//printf("\n");
	//printf(" bss_begin={%x}\n", &bss_begin );
	//printf("   bss_end={%x}\n", &bss_end );
	
	//printf("\n");
	//refresh_screen();
	//die();
	
	
	
	// ## testando ctype ##
    //ok ... isso funcionou.
	
   /*	
   int var1 = 'h';
   int var2 = '2';
    
   if( isdigit(var1) ) {
      printf("var1 = |%c| is a digit\n", var1 );
   } else {
      printf("var1 = |%c| is not a digit\n", var1 );
   }
   
   if( isdigit(var2) ) {
      printf("var2 = |%c| is a digit\n", var2 );
   } else {
      printf("var2 = |%c| is not a digit\n", var2 );
   }
	
	//#debug
	refresh_screen();
	while(1){}
	*/
	

	//
    // RETURNING !
    //

done:

    // Return to assembly file, (head.s).
    if ( KernelStatus == KERNEL_INITIALIZED )
	{

#ifdef KERNEL_VERBOSE
    refresh_screen();
#endif

        //This function do not return.
        startStartIdle() ;
    };

fail:
    printf("x86main: EXIT_FAILURE \n");
	refresh_screen();
    return (int) EXIT_FAILURE;
};


//
// End.
//

