/*
 * Gramado Kernel - The main file for the kernel.
 * (c) Copyright 2015-2016 Fred Nora.
 *
 * File: k\main.c 
 * 
 * Description:
 *     project browser: 'SHELL.BIN IS MY MASTER.'
 *     This is the Kernel Base. It's the mains file for a 32bit Kernel. 
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
 *      + kMain - The entry point for a C part of the Kernel.
 *      + Nothing!
 *
 * Revision History:
 *     Version: 1.0, 2015 - Created by Fred Nora.
 *     Version: 1.0, 2016 - Revision.
 *     //... 
 */ 
#include <kernel.h>


// 
// Variables from Boot Loader.
//

extern unsigned long SavedBootBlock;    //Boot Loader Block.
extern unsigned long SavedLFB;          //LFB address.  
extern unsigned long SavedX;            //Screen width. 
extern unsigned long SavedY;            //Screen height.
extern unsigned long SavedBPP;          //Bits per pixel.
//...

//
// Args.
//

extern unsigned long kArg1;
extern unsigned long kArg2;
extern unsigned long kArg3;
extern unsigned long kArg4;
//...
 

extern unsigned long SavedBootMode;




/*
 * kMain: 
 *     The entry point for a C part of the Kernel.
 *
 * Function history: 
 *     2015 - Created.
 *     Nov 2016 - Revision.
 *     ... 
 */
int kMain(int argc, char* argv[])
{	
    int Status = 0;

    KernelStatus = KERNEL_NULL;
	
	//Inicializando a flag que será usada para travar o foco
	// de entrada na tela do desenvolvedor. gui->DEVELOPERSCREEN
	//@todo: Não usar isso.
	//_lockfocus = 0; 
	
	//Inicializando o semáforo binário do kernel de uso geral.
	//Aplicações usarão isso para entrarem em suas sessões críticas.
	//@todo: Criar um método para isso.
    __ipc_kernel_spinlock = 1;
	
	
    //@todo: Encontrar o lugar apropriado para a  inicialização de zorder 
	//provavelmente na inicialização da gui.
	// initializing zorder list
	int zIndex;
	for( zIndex = 0; zIndex < ZORDER_COUNT_MAX; zIndex++ ){
		zorderList[zIndex] = (unsigned long) 0;
	}
	
	//initializing zorder counter.
	zorderCounter = 0;
	
	//
	// Window procedure.
	//
	
	SetProcedure( (unsigned long) &system_procedure);
	
	
	//
	// Video.
	//
	
//setupVideo:	
	
	//@todo: Device screen sizes.
	
    //Set graphic mode or text mode using a flag.
	if(SavedBootMode == 1){
        g_useGUI = GUI_ON; 
	    VideoBlock.useGui = GUI_ON;
        //... 		
	}else{
		g_useGUI = GUI_OFF;  
		VideoBlock.useGui = GUI_OFF;
        //...		
	};

	//Construtor. 
	//Configura algumas variáveis internas do gerenciador de vídeo.
	videoVideo();
	
	videoInit();  //Setup video first of all.	   
	

	//
	// Init screen
	//
	
	//If we are using graphic mode.
	if(VideoBlock.useGui == GUI_ON){
#ifdef KERNEL_VERBOSE	
		//kbackground(COLOR_BLUE);
	    printf("kMain: Using GUI!\n");	
        //drawDataRectangle( 20, 20, 20, 20, COLOR_RED);
#endif       	
	};
	
	//If we are using text mode.
	if(VideoBlock.useGui != GUI_ON){	
	    set_up_text_color(0x0F, 0x00);
		
		//g_current_vm = 0x800000;
	    //Set cga video memory: 0x800000vir = 0xb8000fis.
	    videoSetupCGAStartAddress(SCREEN_CGA_START); 
	    
	
	    //Debug.
	    kclear(0);
	    kprintf("kMain: Debug" ,9 ,9 );
	    printf("kMain: Text Mode!\n");	    	
	};
	
	//Debug message.
#ifdef KERNEL_VERBOSE
	printf("kMain: Starting up system..\n");
	refresh_screen(); //@TODO: Talvez isso não precise.
	//while(1){}	
#endif
	
	//
    // System initialization..
    //
    
//initializeSystem:
	
	//Construtor.
    systemSystem();	
	
	//Inicializações.
	Status = (int) systemInit();    //(system.c).	
    if(Status != 0){	
		printf("kMain fail: System Init.\n");
		KernelStatus = KERNEL_ABORTED; 
		goto fail;		
	};
	
	//
	// Creating processes and threads.
	// The processes are: Kernel, Idle, Shell, Taskman.
	//
	
//createProcesses:	
	
	// Creating Kenrel process. PID=0.	
	KernelProcess = (void*) KiCreateKernelProcess();	
	if( (void*) KernelProcess == NULL ){
	    printf("kMain error: Create KernelProcess!");
		refresh_screen();
		while(1){}
	}else{
		//ppid.
		KernelProcess->ppid = (int) KernelProcess->pid;
        
		//process.
	    processor->CurrentProcess = (void*) KernelProcess;
        processor->NextProcess    = (void*) KernelProcess;	
		//...
    };

    //Creating Idle process.
	IdleProcess = (void*) create_process( NULL, NULL, NULL, (unsigned long) 0x00401000, 
	                                      PRIORITY_HIGH, (int) KernelProcess->pid, "IDLEPROCESS");	
	if((void*) IdleProcess == NULL){
		printf("kMain error: Create Idle process.\n");
		die();
		//refresh_screen();
		//while(1){};
	};
    //processor->IdleProcess = (void*) IdleProcess;	
	
    //Creating Shell process.
	ShellProcess = (void*) create_process( NULL, NULL, NULL, (unsigned long) 0x00401000, 
	                                       PRIORITY_HIGH, (int) KernelProcess->pid, "SHELLPROCESS");	
	if((void*) ShellProcess == NULL){
		printf("kMain error: Create shell process.\n");
		die();
		//refresh_screen();
		//while(1){};
	};	

	//Creating Taskman process.
	TaskManProcess = (void*) create_process( NULL, NULL, NULL, (unsigned long) 0x00401000, 
	                                         PRIORITY_LOW, 0, "TASKMANPROCESS");	
	if((void*) TaskManProcess == NULL){
		printf("kMain error: Create taskman process.\n");
		die();
		//refresh_screen();
		//while(1){};
	};		
	
	//
	// Creating threads. 
	// The threads are: Idle, Shell, Taskman.
	// The Idle thread belong to Idle process.
	// The Shell thread belongs to Shell process.
	// The Taskman thread belongs to Taskman process.
	//

//createThreads:
	
	//Create Idle Thread. tid=0. ppid=0.
	IdleThread = (void*) KiCreateIdle();	
	if( (void*) IdleThread == NULL )
	{
	    printf("kMain error: Create Idle Thread!");
		die();
		//refresh_screen();
		//while(1){}
	}else{
	    
        IdleThread->ownerPID = (int) IdleProcess->pid;  //ID do processo ao qual o thread pertence.    
		
		//Thread.
	    processor->CurrentThread = (void*) IdleThread;
        processor->NextThread    = (void*) IdleThread;
        processor->IdleThread    = (void*) IdleThread;		
		//...		
    };
	
	// Create shell Thread. tid=1. 
	ShellThread = (void*) KiCreateShell();	
	if( (void*) ShellThread == NULL ){
	    printf("kMain error: Create Shell Thread!");
		die();
		//refresh_screen();
		//while(1){}
	}else{
		
	    ShellThread->ownerPID = (int) ShellProcess->pid;  //ID do processo ao qual o thread pertence. 
		//...		
    };
	
	//Create taskman Thread. tid=2. 
	TaskManThread = (void*) KiCreateTaskManager();
	if( (void*) TaskManThread == NULL ){
	    printf("kMain error: Create TaskMan Thread!");
		die();
		//refresh_screen();
		//while(1){}
	}else{
		
	    TaskManThread->ownerPID = (int) TaskManProcess->pid; //ID do processo ao qual o thread pertence. 		
		//...		
    };	
	
 
    //
	// Debug.
	//
	
doDebug:
	
	//Kernel base Debugger.
	Status = (int) debug();	
	if(Status != 0){    
		//printf("kMain fail: Debug!\n");
		MessageBox(gui->screen,1,"kMain ERROR","Debug Status Fail!");
		KernelStatus = KERNEL_ABORTED;	
		goto fail;
	}else{
	    KernelStatus = KERNEL_INITIALIZED;
	};	
	
	

	//
	// TESTS:
	// We can make some testes here.
	//
//doTests:	
   //...
    
	//
	// Done.
	//
void *b;

done:

	//if(VideoBlock.useGui != 1){	
	//    kclear(0);
    //    kprintf("kMain: Done" ,10 ,9 );
    //};
	
	
	
	//
	// *Importante:
	//  Iniciando o suporte ao browser.
	//  O Browser é uma janela criada e gerenciada pelo kernel ...
	//  em suas abas rodarão os aplicativos e páginas da web.
	//
	
	/* suspendido
	printf("main: initializing browser support.\n");
	windowInitializeBrowserSupport();
	printf("main: done.\n");
	*/
	
	
	//
	// Tentando inicializar o controlador de mouse.
	//
	// Já foi criada a irq12
	// ja foi configurado vetor da IDT.
	// já foi criado um handle para a irq12.
	// agora inicializaremos o controlador 8042
	// testando inicializar o mouse no procedimento de janela do sistema em F6.
	
	//init_mouse();  //isso está em keyboard.c
	
	
	//
	// testing BMP support
	//
	
	//32KB
	
	b = (void*) malloc(512*1024);
	
	unsigned long fileret;
	
	
	taskswitch_lock();
	scheduler_lock();
	fileret = fsLoadFile( "FREDNORABMP", (unsigned long) b);
	if(fileret != 0)
	{
		//escrevendo string na janela
	    draw_text( gui->main, 10, 500, COLOR_WINDOWTEXT, "FREDNORA BMP FAIL");  	
	}
	bmpDisplayBMP( b, 30, 450, 128, 128 );
	scheduler_unlock();
	taskswitch_unlock();
	
	
	
	//
	// RETURNING !
	//
	
	//
	// Return to assembly file, (head.s).
	//
	
	if(KernelStatus == KERNEL_INITIALIZED)
	{

#ifdef KERNEL_VERBOSE
		//printf("KeMain: EXIT_SUCCESS\n");
		refresh_screen();
#endif	
	    return (int) EXIT_SUCCESS;	
	};
	
	// Fail!
fail:
    //if(KernelStatus != KERNEL_INITIALIZED){...};	
	//printf("KeMain: EXIT_FAILURE\n");
	MessageBox(gui->screen,1,"kMain ERROR","Kernel main EXIT_FAILURE!");
	refresh_screen();  
	return (int) EXIT_FAILURE;   
};

 
//
// End.
//

