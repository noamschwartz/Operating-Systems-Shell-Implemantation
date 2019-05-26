//Noam Schwartz
//200160042

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <stdlib.h>
#include <memory.h>

struct job {
    int pid;
    char** jobArgs;
};
/*
 * this function duplicates the buffer using malloc to new memory is allocated
 * and the pointers will be different.
 */
char** duplicateArgs (char ** finalBuffer){
    int numOfArgs = 0;
    //check how many args in the buffer
    while (finalBuffer[numOfArgs] != NULL){
        numOfArgs++;
    }
    //allocate a new buffer with the size of the previous buffer.
    char ** newBufferCopy = malloc(numOfArgs * sizeof(char*));
    int i;
    for (i = 0; i < numOfArgs; i++){
        //built-in duplicate function.
        newBufferCopy[i] = strdup(finalBuffer[i]);
    }
    return newBufferCopy;
}
/*
 * this function adds a single job to the list of jobs.
 * it will be called if there was an & after the args.
 */
void addSingleJob (int pid, char** jobArg, struct job* jobs){
    char** copyBuffer = duplicateArgs(jobArg);
    struct job job = {pid, copyBuffer};
    int index = 0;
    //find an empty index in the buffer to insert the new job.
    int i;
    for (i = 0; i< sizeof(jobs); i++){
        if (jobs[i].jobArgs == NULL){
            break;
        }
        index ++;
    }
    //insert the new job.
    jobs[index] = job;
}
/*
 * this function will remove the last job from the end to the deleted jobs index.
 */
void moveLastJob (int jobToDelete, int lastJob, struct job* jobs){
    //place the last jobs title and id in the deleted jobs place.
    jobs[jobToDelete].jobArgs = jobs[lastJob].jobArgs;
    jobs[jobToDelete].pid = jobs[lastJob].pid;
    //clear the last jobs place in the job array.
    jobs[lastJob].pid = 0;
    jobs[lastJob].jobArgs = NULL;
}

/*
 * this will remove a singe job from the jobs list.
 */
void removeSingleJob (int pidToDelete, struct job* jobs) {
    int lastJob = 0;
    int JobToDelete = 0;
    int jobsSize = sizeof(jobs);
    //find the index of the last job.
    for (lastJob = 0; lastJob < jobsSize; lastJob++) {
        if (jobs[lastJob].jobArgs == NULL) {
            break;
        }
    }
    //find the index of the job we want to delete using its id.
    for (JobToDelete = 0; JobToDelete < jobsSize; JobToDelete++) {
        if (jobs[JobToDelete].pid == pidToDelete) {
            break;
        }
    }
    lastJob -= 1;
    //free allocated memory in the new duplicated buffer.
    free(jobs[JobToDelete].jobArgs);
    //move the last job to the deleted jobs index.
    moveLastJob(JobToDelete, lastJob, jobs);
}

/*
 * this will print a single job to the screen, printing its id, and all of this args.
 */
void printSingleJob (int p, struct job* jobs){
    //find the index of the printed process.
    int jobToPrint = 0;
    int sizeOfJobs = sizeof(jobs);
    //run through the job array and find a job by its id.
    for (jobToPrint = 0; jobToPrint < sizeOfJobs; jobToPrint++) {
        if (jobs[jobToPrint].pid == p) {
            break;
        }
    }
    if (jobs[jobToPrint].jobArgs == NULL){
        return;
    }
    //print the jobs id.
    printf("%d ", jobs[jobToPrint].pid );
    //this will make sure the output is like the format asked for.
    fflush(stdout);
    char ** temp = jobs[jobToPrint].jobArgs;
    int sizeOfargs = sizeof(jobs[jobToPrint].jobArgs);
    int i;
    for (i=0 ; i< sizeOfargs; i++){
        //when an empty arg is reached (in case the array is not full)
        if (((jobs[jobToPrint].jobArgs)[i]) == NULL) {
            break;
        }
        //print the job title.
        printf("%s ", temp[i]);
        //this will make sure the output is like the format asked for.
        fflush(stdout);
        if (((jobs[jobToPrint].jobArgs)[i]) == NULL ){
            break;
        }
    }
    //print a new line between jobs.
    printf("\n");
}
/*
 * this checks is there is a " sign at the begining and end of the arguments.
 */
int checkForQuots(char *buffer, char **finalArgs, int i){
    char *check = strchr(buffer, '"');
    if(check!=NULL){
        buffer = strtok(buffer, "\"");
        finalArgs[i] = buffer;
        return 1;
    }
    return 0;
}
/*
 * this function gets a buffer recieved from the user and breaks it up into a new buffer, putting
 * each argument in its own index.
 */
int makeBuffer(char* buffer, char** finalBuffer) {
    int shouldRunOnBackground = 0;
    const char space[2] = " ";
    char *token = NULL;
    char *check = strchr(buffer, ' ');
    int buffLen = (int) strlen(buffer);
    if (buffer[buffLen - 1] == '\n') {
        buffer[buffLen - 1] = 0;
    }
    //checking if process should run in background
    buffLen = (int) strlen(buffer);
    if (buffer[buffLen-1] == '&') {
        shouldRunOnBackground = 1;
        //converting the space and the & at the end of the string to 0
        buffer[buffLen - 1] = 0;
        buffer[buffLen - 2] = 0;
    }
    int index = 0;
    //if there is more than one argument
    if (check != NULL) {
        token = strtok(buffer, space);
        finalBuffer[index] = token;
        ++index;
        token = strtok(NULL, "\0");
        while (token != NULL) {
            if(!checkForQuots(token, finalBuffer, index)){
                finalBuffer[index] = token;
            }
            token = strtok(NULL, " ");
            index++;
        }
        finalBuffer[index] = NULL;
    }
    else{
        finalBuffer[0] = buffer;
        finalBuffer[1] = NULL;
    }
    return shouldRunOnBackground;
}
/*
 * this function determines if the process should run in the background or not.
 * it determines it by if there is a & at the end of the command. It returns 1 if it should and 0 otherwise.
 */
int ifRunOnBackground (char** finalBuffer){
    int index = 0;
    int bufferSize = sizeof(finalBuffer);
    //find the place of the end of the arguments.
    int i ;
    for (i  = 0 ; i < bufferSize; i++){
        if (finalBuffer[i] == '\0'){
            break;
        }
        index ++;
    }
    char* lastToken = finalBuffer[index - 1];
    //check if the last token is & or not.
    if (strcmp(lastToken, "&") == 0){
        finalBuffer [index - 1 ] = NULL;
        return 1;
    }
    //there is no & at the end of the line.
    return 0;
}

/*
 * this function executes the programs args.
 */
int execute (char** finalBuffer, int runOnBackground, struct job* jobs){
    // fork the process
    pid_t pid = fork();

    // if the forking failed
    if (pid == -1) {
        printf("Failed to fork");
        return 1;
    }

    // if the forking seceded, run the child process
    else if (pid == 0) {
        if(execvp(finalBuffer[0], finalBuffer) < 0 ) {
            // error has accured during execution.
            fprintf(stderr, "Error in system call\n");
            return 0;
        }
        return 1;
    }

        // the parent process
    else if (runOnBackground != 1) {
       //print the id.
        printf("%d\n", pid);
        int childStatus;
        // wait for the childs process to finish
        waitpid(pid, &childStatus, 0);
        return 1;
    }
    //if there is a & at the end of the line this adds the job to the job list.
    addSingleJob(pid, finalBuffer, jobs);
    printf("%d\n", pid);
    return 1;
}

/*
 * this is the programs main function. It gets the input from the user and runs is through the program.
 */
int main() {
    struct job jobs[512];
    while (1) {
        char buffer[513] = { };
        char *finalBuffer[513] = { };
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        int shouldRunOnBackground = makeBuffer(buffer, finalBuffer);
        //if the command is to exit.
        if (strcmp(finalBuffer[0], "exit") == 0) {
            printf("%d\n", getpid());
            break;
        }
        //if the command is to change a directory.
        else if (strcmp(finalBuffer[0], "cd") == 0){
            printf("%d\n", getpid());
            if (finalBuffer[1] !=NULL && strcmp(finalBuffer[1], "-") == 0){
                char cwd[513] = { };
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    int k = (int) (strlen(cwd) - 1);
                    while (cwd[k] != '/'){
                        cwd[k] = '\0';
                        k--;
                    }
                    cwd[k] = '\0';
                    chdir(cwd);
                } else {
                    perror("getcwd() error");
                }
            }
            else {
                if (finalBuffer[1] == NULL || strcmp(finalBuffer[1], "~") == 0 || strcmp(finalBuffer[1], "~/") == 0) {
                    chdir(getenv("HOME"));
                } else if ( chdir(finalBuffer[1]) == -1){
                    fprintf(stderr, "error");
                }
            }
        }
        //if the command is to show all of the jobs running in the background
        else if (strcmp(finalBuffer[0], "jobs") == 0) {
            int lastJob = 0;
            int jobsSize = sizeof(jobs);
            for (lastJob = 0; lastJob < jobsSize; lastJob++) {
                if (jobs[lastJob].jobArgs == NULL) {
                    break;
                }
            }
            //this checks if the process has finished to execute. If so, it removes it.
            int i;
            for (i = 0; i < lastJob; i++) {
                int status;
                pid_t pidToReturn = waitpid(jobs[i].pid, &status, WNOHANG);
                if (pidToReturn != 0) {
                    removeSingleJob(jobs[i].pid, jobs);

                }
            }
            //This checks if the process has finished. If not, it prints it.
            int j;
            for (j = 0; j < lastJob; j++) {
                int status;
                pid_t pidToReturn = waitpid(jobs[j].pid, &status, WNOHANG);
                if (pidToReturn == 0) {
                    printSingleJob(jobs[j].pid, jobs);
                }
            }

        }
        // this sends the command requested by the user to execute.
        else if (execute(finalBuffer, shouldRunOnBackground, jobs ) == 0)
            break;

    }
    return 0;
}