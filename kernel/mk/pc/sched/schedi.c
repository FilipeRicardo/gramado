/*
 * File: schedi.c
 *
 * Descri��o:
 *     Scheduler (Internal).
 *     Chama rotinas de scheduler dentro do kernel base. (internal).
 *     Faz parte do Process Manager, parte fundamental do Kernel Base.
 *     Rotinas de apoio ao m�dulo scheduler.
 *
 * History:
 *     2015 - Created by Fred Nora.
 *     2016 - More basic functions.
 *     2018 - More basic functions.
 *     //...
 */


#include <kernel.h>



//#todo 
//vamos apenas lidar com a estrutura de processo.
//#importante: o kernel ter� a obriga��o de 
//finalizar o processo quando sua thread prim�ria ficar em zombie. 
//poi um processo tem v�rias threads, e se uma fechar n�o significa que o 
//processo terminou. Por isso vamos apenas checar o estado dos processos e 
//n�o das threads.
//rever essa fun��o, h� muito o que fazer nela 
//ela precisa retornar valores padronizados e configurar 
//o status recebido

int do_wait ( int *status ){
	
    int i;
	
	int pid;
	
	struct process_d *p;   

	//#todo
	//inicializando
	*status = 0;
	
    for (i=0; i<PROCESS_COUNT_MAX; i++)
    {
		p = (struct process_d *) processList[i];
		
		if ( (void *) p != NULL )
		{
			if( p->used == 1 && p->magic == 1234 )
			{
				//estado do processo.
				//os processos possuem sua pr�pria lista de estados,
				//diferentes dos estados das threads.
				
			    if ( p->state == PROCESS_TERMINATED )
                {
					//se o seu processo pai for o processo atual.
					
					if ( current_process == p->ppid )
					{
						//aqui precisamos dar informa��es sobre o status 
						//do processo
						*status = 1; //fake value. 
						
					    return (int) p->pid;	
					}
				}					
			}
		}
	};
	
	//aqui precisamos dar informa��es sobre o status 
	//do processo
	*status = 1; //fake value. 

    return -1;	
};


//bloqueia por um motivo
void block_for_a_reason ( int tid, int reason ){
	
	struct thread_d *t;
	
    if ( tid < 0 || tid >= THREAD_COUNT_MAX ){
		return;
	} 
	
	if( reason < 0 || reason >= 8 ){
		return;	
	} 	
	
	//thread
	t = (struct thread_d *) threadList[tid];
	
	if ( (void *) t == NULL )
	{	
        return;
		
	} else {
		
		if ( t->used == 1 && t->magic == 1234 )
		{
			t->wait_reason[reason] = 1;
	        
			//
			// ## Block ##
			//
			
			do_thread_sleeping (tid);		
		}	
	};
};

 

//acordar uma determinada thread se ela estiver 
//esperando por um evento desse tipo.
//uma outra fun��o pode fazer um loop acordando 
//todas as threads que esperam pelo memso motivo.

int wakeup_thread_reason ( int tid, int reason ){
	
	struct thread_d *t;
	
    if ( tid < 0 ||
         tid >= THREAD_COUNT_MAX )
	{
	    goto fail;		
	} 
	
	if( reason < 0 ||
	    reason >= 8 )
	{
	    goto fail;		
	} 
	
	//thread
	t = (struct thread_d *) threadList[tid];
	
	if ( (void *) t == NULL )
	{	
		goto fail;
		
	} else {
		
		if ( t->used != 1 || t->magic != 1234 )
		{
			goto fail;
		}
		
		// OK.
		
		if ( t->wait_reason[reason] != 1 )
		{
		    
			goto fail;
		
		} else {
			
            //ok 
			
			switch (reason)
			{
				//Esperando um processo morrer. Mas qual ??
				case EVENT_WAIT4PID:
				    //se o processo que acabamos de fechar for o mesmo que 
					//a thread estava esperando.
					if ( current_dead_process > 0 && 
					     current_dead_process == t->wait4pid )
					{
					    t->wait_reason[reason] = 0;	
					    KiDoThreadRunning (tid);
					}
					break;
					
				//Esperando uma thread morrer. Mas qual ??
				case EVENT_WAIT4TID:
				    //se a thread que acabamos de fechar for a mesma que 
					//a thread estava esperando.				
					if ( current_dead_thread > 0 && 
					     current_dead_thread == t->wait4tid )
					{
					    t->wait_reason[reason] = 0;	
					    KiDoThreadRunning (tid);
					}
					break;
			    
                // ...				
			}
			
            goto done;
		}
		
		//nothing
	};
	
fail:
    return (int) 1;	
done:
    return (int) 0;	
};




// Acorda todas as threads da lista que est�o esperando por 
// evento de determinado tipo.

int wakeup_scan_thread_reason ( int reason ){
	
    int i;
	
	if ( reason < 0 ||
	     reason >= 8 )
	{
	    goto fail;		
	} 
	
	for ( i=0; i<THREAD_COUNT_MAX; i++ )
	{
        wakeup_thread_reason ( i, reason );		    	
	}
	
	
	switch (reason)
	{
		//j� acordamos todas as threads que esperavam por essa raz�o.
		//podemos sinalizar que n�o h� mais nenhum processo morto recentemente.
		case EVENT_WAIT4PID:
			current_dead_process = 0;
			break;
					
		//j� acordamos todas as threads que esperavam por essa raz�o.
		//podemos sinalizar que n�o h� mais nenhuma thread morta recentemente.
		case EVENT_WAIT4TID:
    		current_dead_thread = 0;
			break;
			    
                // ...				
	};	
	
done:
    return (int) 0;		
fail:
    return (int) 1;	
};



/*
 *************************************************
 * KiScheduler:
 *    Interface para chamar a rotina de scheduler.
 *    Troca as threads que est�o em user mode, 
 * usando o m�todo cooperativo. 
 * Round Robing. 
 *    As tarefas tem a mesma prioridade.
 *    + Quando encontrada uma tarefa de maior prioridade, 
 * escolhe ela imediatamente.
 *    + Quando encontrar uma tarefa de menor prioridade, 
 * apenas eleva a prioridade dela em at� dois valores 
 * acima da prioridade base, pegando a pr�xima tarefa. 
 *    + Quando uma tarefa est� rodando � dois valores 
 * acima da sua prioridade, volta a prioridade para a 
 * sua prioridade b�sica e executa.
 *
 * @todo: 
 *     Essa rotina deve ter retorno do tipo 'int'.
 */
 
int KiScheduler (){
    
	// ?? Quem est� chamando ? Filtros ?
    // @todo: Talvez haja mais casos onde n�o se deva trocar a tarefa.

	//#bugbug 
	//Porque retornamos 0 ???
	//Scheduler Status. (LOCKED, UNLOCKED).
	
	if (g_scheduler_status == LOCKED)
	{
        return (int) 0;
    };

	// Retornaremos se tivermos apenas um thread rodando.
	// Pois n�o h� o que trocar.
	if ( ProcessorBlock.threads_counter == 1 )
	{ 
		//Se s� temos uma ent�o devemos retornar a idle.
		current_thread = idle;
		return (int) idle;
	};
	
    //Chama o Scheduler.

	return (int) scheduler ();
}; 




 
/* KiSelectNextThread: */ 
/* #todo: rever isso */
int KiSelectNextThread (int current ){
	
	//
    // @todo: Fazer filtros.
	//
	
	//@todo: Max.
	
	if (current < 0)
	{
		current = next_thread;
	}
	
//done:

    return (int) SelectNextThread (current);
};


void KiDoThreadReady (int id){
	
    do_thread_ready(id);
};


void KiDoThreadRunning (int id){
	
    do_thread_running(id);
};


void KiDoThreadSleeping (int id){
	
    do_thread_sleeping(id);
};


void KiDoThreadZombie (int id){
	
	do_thread_zombie(id);
};


void KiDoThreadDead (int id){
	
    do_thread_dead(id);
};

 
/*
 * KiNewTaskScheduler:
 *     ??
 *    @todo: Mudar para KiSchedulerNewThread().
 */
void KiNewTaskScheduler()
{
    //return;    //Cancelada.
};


/*
 * KiDispatchTask:
 *     Executa uma thread pronta pra rodar.
 *     @todo: Mudar para KiDispatchThread(int tid);
 *
 */
void KiDispatchTask()
{
	//...
//done:
    //return;
};



/*
 *****************************************************
 * KiSetCurrent:
 *     Seleciona a thread atual.
 *     @todo: Mudar o argumento para (int tid).
 */
void KiSetCurrent (int id){
	
	//@todo: Filtro.
	
    set_current (id);
};


/*
 * KiGetCurrent:
 *     Pega o tid da thread atual.
 */
 
int KiGetCurrent (){
	
    return (int) get_current ();
};


/*
 * KiGetCurrentTask:
 *     @todo: Modar para KiGetCurrentProcess.
 *     #bugbug: Esse nome � inapropriado.
 */
 
int KiGetCurrentTask (){
	
    return (int) get_current_task(); 
};


/* Obtendo o TID da thread atual. */
int get_current (){
	
	return (int) current_thread;
};


/* #deletar
 * get_current_task:
 *     Pega o id da thread atual.
 *    @todo: Criar scheduleriGetCurrentTID();
 */
int get_current_task (){
	
    return (int) get_current();
};


/* #todo: mudar para set_current_thread ( int tid ). */
void set_current (int id){
	
	if ( id == current_thread ){
		return;
	}
	
	// Limits.
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}
	
	// Nothing ?!
	
	// Done.	

	current_thread = (int) id; 	
};


void do_thread_initialized (int id){
	
    struct thread_d *t; 
	
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}
	
	t = (void *) threadList[id]; 	
	
	if ( (void *) t != NULL )
	{
		//if ( t->used == 1 && t->magic == 1234 )
			
	    t->state = INITIALIZED;
	}
};

/* do_thread_ready: #todo: mudar o argumento para tid */ 
void do_thread_ready (int id){
	
    struct thread_d *t; 
	
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}
	
	t = (void *) threadList[id]; 	
	
	if ( (void *) t != NULL )
	{
		//if ( t->used == 1 && t->magic == 1234 )
			
	    t->state = READY;
	}
};


/* do_thread_running: #todo: mudar o argumento para tid */
void do_thread_running (int id){
	
    struct thread_d *t; 
	
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}
	
	t = (void *) threadList[id]; 	
	
	if ( (void *) t != NULL )
	{
		if ( t->used == 1 && t->magic == 1234 )
		{
		    t->state = RUNNING;	
		}
	};
};


/*
 ************************************************************
 * do_thread_sleeping:
 *     Muda o state de uma thread pra blocked.
 *     @todo: Mudar o nome da fun��o para do_thread_blocked.
 */
void do_thread_sleeping (int id){
	
    struct thread_d *t; 

	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}

    // Struct.
	t = (void *) threadList[id];	
	
	if ( (void *) t != NULL){
	    t->state = BLOCKED;
    }	
};


/* do_thread_zombie: #todo: mudar o argumento para tid */
void do_thread_zombie (int id){
	
    struct thread_d *t; 
	 
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}
	
	t = (void *) threadList[id]; 
	
	if( (void *) t == NULL )
	{
	    return;
	}else{
        
		if ( id != IDLE )
		{
            t->state = ZOMBIE;
        }
    };
};


/* do_thread_dead: #todo: mudar o argumento para tid */
void do_thread_dead (int id){
	
    struct thread_d *t; 
	
	if (id < 0 || id >= THREAD_COUNT_MAX){
	    return;
	}

	t = (void *) threadList[id]; 	
	
	if ( (void *) t != NULL )
	{
	    t->state = DEAD;
	}
};


/*
 **************************************************
 * wakeup_thread: 
 *    Para acordar uma thread, basta colocar ela no 
 * estado RUNNING se ela estiver com seu contexto 
 * salvo e seu estado WAITING. 
 *
 * @todo:
 *     Criar scheduleriWakeupYhread(int tid);
 * 
 */
void wakeup_thread (int tid){
	
    int Status;
    struct thread_d *t; 	
	//...
	
	//Limits.
	if ( tid < 0 || tid >= THREAD_COUNT_MAX ){
	    return;
	}

    //Struct.
	t = (void *) threadList[tid]; 
	
	if ( (void *) t == NULL )
	{ 	
	    return;    
	
	}else{
	    
		//Se o contexto n�o foi salvo. 
		//N�o tem como acorda-la.
		//Pois acordar significa apenas retornar 
		//ao estado RUNNING.
		if ( t->saved == 0 ){ 
	        return; 
		}
		
		// Se estiver bloqueada, 
		// n�o tem como acordar ainda. 
		// precisa desbloquear.
		if (t->state == BLOCKED){ 
	        return; 
		}
			
	    //Isso acorda a thread,
		//mas n�o coloca ela imediatamente para rodar,
		//nem precisa.
	    do_thread_running(tid);
	
	};

    // Nothing ?!
};
 

/*
 ***************************************
 * SelectNextThread:
 *     Configurando a pr�xima thread da thread
 * indicada via argumento.
 *
 * @todo: Mudar para schediSelectNextThread(int current);.
 *
 */
int SelectNextThread (int current){
	
	int Next;
	
    struct thread_d *t; 
    struct thread_d *n; 
    
	//Limits.
	
	if ( current < 0 || current >= THREAD_COUNT_MAX ){
		
	    return (int) 0;
	}
	
    // Struct.
	
    t = (void *) threadList[current];		
	
	if( (void *) t == NULL )
	{
	    return (int) 0;
	
	}else{
		
        //Se n�o existe indica��o de pr�xima.
	    if( (void *) t->Next == NULL )
	    {
		    //Procura a pr�xima tarefa, de acordo com a prioridade.
		    Next = (int) find_higher_priority();
			
		    n = (void *) threadList[Next];
		    
			if ( (void *) n == NULL )
			{
				Next = next_thread;
			    return (int) Next; 
		    
			}else{
				
			    Next = (int) n->tid;    //Pega id.
			    return (int) Next;
			};
	    
		//Aceita a indica��o de pr�xima.
		}else{
			
			n = (void *) t->Next;
			
			if ( (void *) n == NULL )
			{
				Next = next_thread;
			    return (int) Next;
			
			}else{
			    Next = (int) n->tid;    //Pega o tid.
				t->Next = NULL;
                return (int) Next;
			};
			//Nothing.
		};
        //Nothing.		
	};
	
	// More ?!
	
// Done.
done:	
    return (int) Next;
};



/*
 *****************************************
 * check_for_standby:
 *     Procura na lista de threads no 
 * estado StandyBy.
 * Se tiver uma thread nessa lista, ela se torna 
 * a current. Para rodar pela primeira vez, 
 * atrav�z de Spawn.
 * N�o retorna se encontrar uma threa na lista.
 *
 */
 
void check_for_standby (){
	
    int newId;
	
	struct thread_d *New;
	
	
	// #todo: 
	// Max variable.
	
	int i = 0;
	int Max = 32;
	
	
	debug_print(" check_for_standby ");
	
	
	do 
	{
		New = (void *) queue->standbyList[i];
		
		if ( (void *) New != NULL )
		{
		    if ( New->used == 1 && 
			     New->magic == 1234 && 
				 New->state == STANDBY )   
			{
			    	
				current_thread = (int) New->tid;
			    goto do_spawn;
			
			};
		};
		
		++i;
		
	} while (i < Max);  

    // Done. 
    // Nenhuma tarefa precisa ser inicializada.
    // Podemos apenas retornar para o taskswitch.
	
    //done:
	
	debug_print(" nothing ");
    
	return;
	
	
	//
	//  ## SPAWN ##
	//
	
    // Do Spawn.
    // spawn.c
	
do_spawn:
	
    debug_print(" SPAWN \n");
    KiSpawnTask ( current_thread );
	
    //  ## No return ##	
	
    panic ("schedi-check_for_standby: ERROR");	
};


/*
 *****************************************************
 * check_quantum:
 *     Checa o quantum atribuido �s threads da lista.
 *     Seleciona a primeira thread encontrada com o 
 * quantum no limite. #todo: rever isso.
 *
 * @todo: Mudar para schediCheckThreadQuantum();
 *
 */
int check_quantum (){
	
    //int newId;	
	
	struct thread_d *New;
	
	int i=0;
	
	while ( i < THREAD_COUNT_MAX )
	{
        New = (void *) threadList[i];    	
	    
		if ( (void *) New != NULL )
		{ 
		    if ( New->used == 1 && 
				 New->magic == 1234 &&
				 New->state == READY &&
				 New->quantum == QUANTUM_LIMIT )
			{
		        return (int) New->tid;
			};
			//Nothing.
		};	
		
		i++;
	};
	
	// Nothing ?!
	
    return (int) 0;
};


//
// End.
//

