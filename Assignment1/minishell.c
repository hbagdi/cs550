#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "minishell.h"

#define INP_SIZE_MAX    1024 //max chars in a cmd
#define ARGS_MAX 10	// max args per cmd
#define TOKENS_MAX  INP_SIZE_MAX	// maximum number to tokens in cmd line
#define CHILD_PER_CMD_MAX 100	//max number of cmds in a pipe
typedef void Sigfunc(int);
#define HARDIK_CODE

#ifdef HARDIK_CODE
//bash commands
char *exit_cmd  ="exit";
char *jobs_cmd  ="jobs";
char *cd_cmd  ="cd";
char *fg_cmd  ="fg";
//handling termination
pid_t last_started_process = 0;
char *inp_backup;
static char *background_operator = "&";
int is_background = 0;
#endif
pid_t current_fg_pg_id;
pid_t group_id; 
char inp[INP_SIZE_MAX]; //cmd line
char *inp_tokens[TOKENS_MAX]; // token list of entire command line
struct child_process {
	char cmd_start_index; // index of the first token for this cmd in cmd line
	char cmd_len;	//number of command line arguments for this process( including cmd name)
	pid_t pid;	//pid of this child
    int pipe_out;
    int pipe_in;
    int fds[2];
} child_list[CHILD_PER_CMD_MAX];	// a cmd line can have at most

int num_childs;	// total number of child processes in this command 

pid_t shell_pid;    //Pid of shell

#ifdef HARDIK_CODE
//DS to hold jobs, implemented as LinkedList
typedef struct Job{
    pid_t pid;
    char* cmd;
    struct Job* next;
}Job;
//jobs linked list
Job *jobs;
int no_of_jobs = 0;
Job* createJob(char* cmd, pid_t pid){
    Job* job = calloc(sizeof(Job),1);
    char* pos;
    char* cmd_copy = calloc(strlen(cmd),1);
    if(job == NULL || cmd_copy == NULL){
        printf("Out of Memory\n");
        fflush(stdout);
        exit(0);
    }
    memcpy(cmd_copy,cmd,strlen(cmd));
    if ((pos=strchr(cmd_copy, '\n')) != NULL)
        *pos = '\0';

    job->pid = pid;
    job->cmd = cmd_copy;
    job->next = NULL;
    return job;
}
int addjob(Job* job){
    no_of_jobs++;
    if(jobs== NULL){
        jobs=job;
        return 0;
    }
    Job* curr = jobs;
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = job;
    return 0;
}
//wrapper to hold background jobs
void put_into_background(char* inp_backup, pid_t pid){
    // printf("from put_into_bk: %d",pid);
    Job *job = createJob(inp_backup,pid);
    addjob(job);
}
//search by pid if exist. NULL if doesn't exists
Job* search(pid_t pid_to_search){
    Job* curr = jobs;
    while(curr!= NULL && curr->pid!=getpgid(pid_to_search)){
        curr = curr->next;
    }
    return curr;
}
//get background at an index
Job* search_by_index(int index){
    Job* job = jobs;
    while(index-- > 1)
        job = job->next;
    return job;

}
int removejob(pid_t pid_to_remove){
    if(search(pid_to_remove) == NULL)
        return 0;
    Job* curr = jobs;
    if(curr->pid == getpgid(pid_to_remove)){
        Job* temp = jobs;
        jobs = jobs->next;
        --no_of_jobs;
        free(temp);
        return 0;
    }
    Job* prev = jobs;
    curr = jobs->next;
    while(curr!= NULL && curr->pid != pid_to_remove){
        curr = curr->next;
        prev = prev->next;
    }
    if(curr== NULL){
        return 0;
    }
    prev->next = curr->next;
    --no_of_jobs;
    free(curr->cmd);
    free(curr);
    return 0;

}
//removes zombie process from background jobs
void remove_dead_jobs_backup(){
    Job* curr = jobs;
    pid_t pid;
    Job* temp= NULL;
    while(curr!=NULL){
        temp = curr->next;
        pid = waitpid(curr->pid,NULL, WNOHANG);
        if(pid == curr->pid){
            removejob(curr->pid);
        }
        curr = temp;
    }
}
void remove_dead_jobs(){
    Job* curr = jobs;
    pid_t pid;
    Job* temp= NULL;
    while(curr!=NULL){
        temp = curr->next;
        pid = waitpid(curr->pid,NULL, WNOHANG);
        if(pid == curr->pid){
            removejob(getpgid(curr->pid));
        }
        curr = temp;
    }
}

//print used for 'jobs' command
void print_jobs(){
    remove_dead_jobs();
    if(jobs==NULL){
        printf("No background jobs\n");
        fflush(stdout);
        return;
    }
    int i=1;
    Job* curr = jobs;
    while(curr != NULL){
        printf("[%d] Process: %d\t%s\n",i++,curr->pid,curr->cmd );
        curr = curr->next;
    }
    fflush(stdout);
    return;
}

//find length of a command
int length(char** argv){
	int len = 0;
	while(argv[len] != NULL){
		len++;
	}
	return len;
}

int make_process_background(char* cmd, pid_t pid){
    // printf("Back requestd: %d\n",pid );
    put_into_background(cmd,pid);
    //printf("Background process: [%d] %s\n",pid,cmd);
    return 0;
}
int remove_background_operator(char** argv){
    int len  = length(argv);
    argv[len-1] =NULL;
    return 0;
}
int check_if_background(char* inp){
    char *argv[TOKENS_MAX];
    char *inp_copy = calloc(strlen(inp),1);
    memcpy(inp_copy,inp,strlen(inp));
    split_str( inp_copy, INP_SIZE_MAX, argv, TOKENS_MAX  );
    int len = length(argv);
    if(strcmp(argv[len-1],background_operator)==0){
        // printf("background requested yes\n");fflush(stdout);
       return 1;
   }
    free(inp_copy);
   return 0;
}
//end hardik

void wait_for_child(int pid){
    int child_status;
    pid_t pid_returned;
       // tcsetpgrp (STDIN_FILENO,pid);
       // printf("parent  wait\n");
         pid_returned = waitpid(pid, &child_status, WUNTRACED);
       // printf("waitpid: %d\n",pid_returned);
         if(pid == pid_returned && WIFSTOPPED(child_status)){
                // printf("Current process stopped \n");fflush(stdout);
            if(search(pid)==NULL)
                put_into_background(inp_backup,pid_returned);
         }
         else if(pid == pid_returned && WIFEXITED(child_status)){
            if(search(pid_returned))
                removejob(getpgid(pid_returned));
                // printf("Current process terminated normally \n");fflush(stdout);
         }
         else if(pid == pid_returned && WIFSIGNALED(child_status)){
                 // printf("Current process terminated on signal \n");fflush(stdout);
                // tcsetpgrp (STDIN_FILENO, getpid());
            if(search(pid_returned))
                removejob(getpgid(pid_returned));

         }
}

void execute_cd(char* directory){
    char* pos;
    if(directory== NULL){
        printf("Please specify a directory.\n");
    }
    else{
        if(chdir(directory)!=0){
            //printf("Error: %d\n",errno);
            printf("%s\n", strerror( errno ));
        }
    }
    fflush(stdout);
}

void execute_fg(char* job_id){
    if(job_id == NULL || job_id[0]!='%'){
        printf("Syntax: fg %%N\n");
        return;
    }
    int index = atoi(++job_id);
    // printf("%d job id requested\n",index );
    if(index > no_of_jobs){
      printf("Job doesn't exist\n");
      return;
    }
    Job* job_to_resume = search_by_index(index);
    // printf("Pg id to resume:%d\n",job_to_resume->pid );
    current_fg_pg_id = getpgid(job_to_resume->pid);
    if(killpg(getpgid(job_to_resume->pid), SIGCONT)){
        // printf("some error\n" );
    }
    //two calls - first to handle SIGCONT and then to actually wait

    wait_for_child(getpgid(job_to_resume->pid));
    // wait_for_child(getpgid(job_to_resume->pid));
    // wait_for_child(getpgid(job_to_resume->pid));
    //
    //printf("wait returned\n");

}
//checks for exit, jobs, fg and cd command
int check_and_handle_bash_cmd(char* inp){
    //tokenize input
    char *argv[TOKENS_MAX];
    //char *inp_copy = calloc(strlen(inp),1);
    char inp_copy[INP_SIZE_MAX];
   memset( inp_copy, 0 , sizeof(inp_copy)); 
    memcpy(inp_copy,inp,strlen(inp));
    split_str( inp_copy, INP_SIZE_MAX, argv, TOKENS_MAX);
//printf("checking for bash: %s\n", argv[0]);  
    if(argv[0] == NULL)
        return 0;
    else{
        if((strcmp(argv[0],exit_cmd)==0)){
            printf("Shell will exit now\n");fflush(stdout);
            exit(0);
        }
        else if((strcmp(argv[0],cd_cmd)==0)){
            execute_cd(argv[1]);
            return 1;
        }
        else if((strcmp(argv[0],jobs_cmd)==0)){
            printf("Background Jobs:\n");
            print_jobs();
            fflush(stdout);
            return 1;
        }
        else if((strcmp(argv[0],fg_cmd)==0)){
            execute_fg(argv[1]);
            return 1;
        }
        else{
            return 0;
        }
    }
    //free(inp_copy);  
    return 0;
}
Sigfunc *install_signal_handler(int signo, Sigfunc *handler)
{
    struct sigaction act, oact;

    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if( signo == SIGALRM ) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;  /* SunOS 4.x */
#endif
    } else {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;  /* SVR4, 4.4BSD */
#endif
    }

    if( sigaction(signo, &act, &oact) < 0 )
        return (SIG_ERR);

    return(oact.sa_handler);
}
void handle_SIGINT(int sig)
{
 // printf("GOT SIGKILL: %d\n",current_fg_pg_id);
  if(current_fg_pg_id!=0)
    killpg(getpgid(current_fg_pg_id),SIGKILL);
  if(search(current_fg_pg_id))
    removejob(getpgid(current_fg_pg_id));
}
void handle_SIGTSTP(int sig){
  //printf("GOT SIGTSTP: %d\n",current_fg_pg_id);
  if(current_fg_pg_id!=0)
    killpg(current_fg_pg_id,SIGTSTP);
       // printf("killpg in SIGTSTP handler failed\n" );
}
void handle_SIGCHILD(int sig){
 //   printf("got sigchild\n");
}
void init_shell(){
   setpgid(getpid(), getpid());
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGINT, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
     install_signal_handler(SIGINT, handle_SIGINT);
     // install_signal_handler(SIGCHLD, handle_SIGCHILD);
    install_signal_handler(SIGTSTP,handle_SIGTSTP);
}
void init_child_process(){
 //   tcsetpgrp (STDIN_FILENO, getpid());
    // install_signal_handler(SIGINT, handle_SIGINT);
    signal (SIGINT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    // install_signal_handler(SIGTSTP,handle_SIGTSTP);
    // signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
}

#endif
int split_str( char *line, unsigned int line_len, char **tok_list, unsigned int tok_list_len)
{
    int i = 0;
    char *pos;
    if( (line == NULL) || (line_len == 0) || (tok_list == NULL) || (tok_list_len == 0)) {
        printf(" error parsing tokens\n");
        return -1;
    }
    if ((pos=strchr(line, '\n')) != NULL)
        *pos = '\0';
    tok_list[i] = strtok(line," ");

    while( (i < tok_list_len) &&  (tok_list[i] != NULL))  {
            tok_list[++i] = strtok(NULL, " ");
    }
     
}                

int run_child( char *cmd, int len)
{
    char *argv[ARGS_MAX];
    memset(argv, 0, sizeof(argv));
    split_str( cmd, len, argv, ARGS_MAX); 
    //printf("running :%s:\n", argv[0]);
    if( execvp(argv[0], argv) < 0 )  {
    //if( execv("/bin/date", argv) < 0 )  {
        printf("Error %s for cmd %s\n", strerror(errno), argv[0]);
    }
    return 0;
}

int fill_child_list( void )
{
    int i = 0;
    int j = 0;
    child_list[j].cmd_start_index = 0;
    child_list[j].cmd_len = 1;
    for( i = 1; inp_tokens[i] != NULL; i++) {
        //printf("found token :%s:\n", inp_tokens[i]);
        if( *inp_tokens[i] == '|')  {
            j++;
            child_list[j].cmd_start_index = i + 1;
           // child_list[j].cmd_len = 1;
        }else   {
            child_list[j].cmd_len++;
        }
    }
    num_childs = j+1;
    //printf("total number of childs = %d\n",num_childs);
    return 0;	
}

int current_child = -1;  //index to the child_list[]

int run_exec(void)
{
    char *argv[ARGS_MAX];
    int i = 0, j = 0;
    int in_fd;
    int out_fd;
    char* in_file = NULL;
    char* out_file = NULL;
    
    //printf(" exec for child %d\n", current_child); 
    //printf(" exec for child --"); 
    memset(argv, 0 , sizeof(argv)); //check this 
    //printf("exec for child %d, cmd_start_index = %d, cmd len = %d\n", \
            current_child, child_list[current_child].cmd_start_index, child_list[current_child].cmd_len);
    for( i = child_list[current_child].cmd_start_index; \
            i < ( child_list[current_child].cmd_start_index + child_list[current_child].cmd_len); i++)  {
    //for( i = child_list[current_child].cmd_start_index; inp_tokens[i] != NULL;  i++)  {
        if(*inp_tokens[i] == '>')    {
            out_file = inp_tokens[i + 1];
            i++;    //skip the next token because it is a fine name
        }   
        else if(*inp_tokens[i] == '<')   { 
            in_file = inp_tokens[i + 1];   
            i++;    //skip the next token because it is a fine name
        }
        else    {
            argv[j++] = inp_tokens[i];
           // printf("inp_tokens[%d] = %s \n",i,  inp_tokens[i]);
        }
       // printf("1HERE \n");
    }
    //printf("HERE \n");
    if( in_file != NULL )   { 
       in_fd = open(in_file, O_RDONLY );
       if( in_fd  < 0 )   {
            printf("Error %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       if( dup2(in_fd,  fileno(stdin)) < 0)   {
            printf("Error in dup %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       close(in_fd);
    }

    if( out_file != NULL )   {
       out_fd = open(out_file, O_CREAT|O_WRONLY|O_TRUNC , 0600);
       if(  out_fd  < 0 )   {
            printf("Error %s for cmd %s, file %s\n", strerror(errno), argv[0], out_file);
            return 1;   
       }
       if( dup2(out_fd, fileno(stdout)) < 0)   {
            printf("Error in dup %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       close(out_fd);
      //fprintf(stderr, "OUTPUT!!!!\n");
    }
/*
    printf("Running:\n");
    int k = 0;
    while( argv[k] != NULL ){
        printf("%s ", argv[k]);
        k++;
    }
    printf("\n");
*/
    if( execvp(argv[0], argv) < 0 )  {
        printf("Error %s for cmd %s\n", strerror(errno), argv[0]);
        exit(0); //Experimental
    }
    printf("never print\n");
    return 0;
}

int create_pipe( int *fds)
{
    if ( pipe(fds) == -1) {
        printf("Pipe creation error %s, for child %d \n", strerror(errno), current_child);
        exit(1);
    }
   return 0;
}

int open_write_end(int *fds)
{
    close(1);  /* close normal stdout (fd = 1) */
    if( dup2(fds[1], 1)) {    /* make stdout same as fds[1] */
        printf("Pipe open write end error %s, for child %d \n", strerror(errno), current_child);
    }
    close(fds[0]); /* we don't need the read end -- fds[0] */ 
    return 0;
}

int open_read_end(int *fds)
{
    close(0);  /* close normal stdin (fd = 0) */
    if( dup2(fds[0], 0)) {    /* make stdin same as fds[0] */
        printf("Pipe open read end error %s, for child %d \n", strerror(errno), current_child);
    }
    close(fds[1]); /* we don't need the write end -- fds[1] */ 
    return 0;
}

int spawn_child( void )
{
    pid_t pid;
    int fds[2]; //For Input and Output redir
//    current_child++;
   // printf("current child = %d\n", current_child);
    // if thre is a pipe
    
    if( num_childs > 1 )    {
        
       // printf("creating pipe in  = %d\n", current_child);
        create_pipe(fds);
        //     and if current child is not the last one in the pipe, create out pipe
        if( current_child < (num_childs -1))    {
            open_write_end(fds);
        }
        if(  current_child > 0 )    {
            open_read_end(fds);
        }
   }
      
    pid = fork();
    if( pid < 0 )   {
        printf("Error1 %s \n", strerror(errno));
        exit(1);
    }
    if( pid == 0 )  {   //child
        current_child++;
        if( current_child < (num_childs - 1))   { // there are more commands in pipe, spawn recursively
            printf("spawning recursive child. cur child = %d\n", current_child);
          // current_child++;
            spawn_child();
        } else{             // last cmd in pipe
           // printf("last command, %d\n", current_child);
            run_exec();
        }
    }
    else    {   //parent
        if( this_is_shell())    {
            wait(NULL); //shell does not exec itself
        }
        else    {
           // sleep(1);
            printf("runnig exec for child  %d\n", current_child);
            run_exec(); // this is a child
        }
    }
    return 0;
}
int spawn_child_itrative( int child )
{
    pid_t pid;
    int fds[2]; //For Input and Output redir
    //printf("Itrative: child = %d\n", child);
    pid = fork();
    if( pid < 0 )   {
        printf("Error1 %s \n", strerror(errno));
        exit(1);
    }
    if( pid == 0 )  {   //child
        if(child == 0 ) {
            setpgid(getpid(), getpid());
        }
        else    {
            setpgid(0, group_id);
        }
        init_child_process();
        current_child = child;
        if(child_list[current_child].pipe_in == TRUE ){
            open_read_end( child_list[current_child - 1].fds);
        }
        if(child_list[current_child].pipe_out == TRUE ){
            open_write_end( child_list[current_child].fds);
        }
       run_exec();
    }
    else {
       last_started_process = pid;
        if( child == 0) {
            group_id = pid;
            current_fg_pg_id = pid;
          //  printf("setting fg grp\n" );
            if(0!=tcsetpgrp(STDIN_FILENO,group_id)){
            printf("(error in tcsetpgrp)\n");
           }
        }
    }    
    return 0;
}

int this_is_shell( void )
{
    pid_t this_pid = 0;
    this_pid = getpid();
    if( this_pid == shell_pid )
        return TRUE;
    else
        return FALSE;
}




int main(void)
{
    int child_status = 0;
    pid_t pid;
    int i = 0;
    int count = 0;
    memset(inp,0,sizeof(inp));    
    //record the pid of shell before spawining children
    shell_pid = getpid();
    init_shell();
    while(1)    {    
        memset( inp_tokens, 0, sizeof(inp_tokens));
        memset( inp, 0, sizeof(inp));

        memset( child_list, 0, sizeof(child_list));
        num_childs = 0;
        current_fg_pg_id=0;
        current_child = -1;  //index to the child_list[]
 
        printf("msh>");
        fflush(stdin);
        fgets(inp, INP_SIZE_MAX, stdin);
#ifdef HARDIK_CODE

        inp_backup = calloc(strlen(inp),1); //backing up input- for background and ctrl+z
        memcpy(inp_backup,inp,strlen(inp));
        //handle bash commands
        if(check_and_handle_bash_cmd(inp)){
		//printf(" bash cmd\n");
            continue;
        }
       is_background = check_if_background(inp);//hardik
#endif
       	//tokanise the cmd line input
    	split_str( inp, sizeof(inp), inp_tokens, sizeof(inp_tokens)); 
	    //fillout child process array
       if( is_background )  {
        remove_background_operator(inp_tokens);
       } 
	    fill_child_list();
        // spawn_child();   // recursive 
        for( i = 0; i < num_childs; i++)    {
            //printf("i = %d", i);
            if( num_childs > 1) {
                if( i < (num_childs -1)) {
                    child_list[i].pipe_out = TRUE;
                    create_pipe( child_list[i].fds );
                }
                if( i > 0 ){
                    child_list[i].pipe_in = TRUE;
                }
            }               
            spawn_child_itrative(i);
            if( child_list[i].pipe_in )
                close(child_list[i-1].fds[0]);
            if( child_list[i].pipe_out )
                close(child_list[i].fds[1]);

        } 
        //for( i = 0; i < num_childs; i++)    {
        //    printf(" child finished %d\n", wait(NULL));
        //}
        // printf("Current fg pgid: %d\n",current_fg_pg_id );
        if(is_background){
            make_process_background(inp_backup,current_fg_pg_id);
        }
        else{
           //waitpid(last_started_process, &child_status, WUNTRACED);
            wait_for_child(last_started_process);
           if(0!=tcsetpgrp(STDIN_FILENO,getpid())){
            printf("(error in tcsetpgrp)\n");
           }
        }

        
       }
	return 0;
}
