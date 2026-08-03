/* A simuilation of a shell. Works similarly to BASH but is called catshell>
*
* Time spent: 5 hours
*
* Authors: RV
*/

#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/wait.h>  
#include <signal.h>

#define MAX_CMD_LENGTH 1000
#define HISTORY 10

//define the struct history with a command and a command ID
typedef struct {
char command[MAX_CMD_LENGTH];
unsigned int commandID;
} history_t;

history_t history[HISTORY];
// define a circular buffer history of structs type history_t with a size of 10

int head = 0; //head of ring buffer
int count = 0; // iterations of ring buffer
unsigned int nextCommandID = 1; // uint for next ID in the ring buffer


/* Adds the supplied command line to the circular history buffer. 
Updates the global history buffer and advances the next ID. 
   Parameters:
       comd - the command string to store (assumed NUL-terminated).
   Returns:
       void
*/
void addToHistory(char* comd){ 

    int slot; // slot in ring buffer
    if(count < HISTORY){
        //if the count we are on is less than 10
        slot = (head + count) % HISTORY;
        //index to go in circular buffer is the head plus count mod 10
        count++;
    }
    else{
        // if the count is equal to 10 (at the tail)
        slot = head; //replace with the first slot in the ring buffer
        head = (head + 1) % HISTORY; 
        //make the next slot over (the new oldest slot) the new head
        
    }

    strncpy(history[slot].command, comd, MAX_CMD_LENGTH - 1);
    //copy the command into history at the given slot in the circular buffer

    history[slot].command[MAX_CMD_LENGTH - 1] = '\0';
    // NULL terminate the the command length is 999

    history[slot].commandID = nextCommandID++;
    //assigns the commanID to the spot in history a new id

}

/* Prints the current history buffer from oldest to newest.
Writes each command ID and string to stdout.
   No parameters
   Returns:
       void
 */
void printHistory() {
    for(int i = 0; i < count; i++) {
        int idx = (head + i) % HISTORY;
        fflush(stdout);// make sure the prompt get printed imediately 
        printf("\t%u %s\n", history[idx].commandID, history[idx].command);
    }
}

/* Frees the NULL-terminated array of tokens produced by parseCommand. Frees each token and the array itself.
   Parameters:
       tokens - array of allocated strings ending with NULL (may be NULL).
   Returns:
       void 
 */
void freeTokens(char** tokens){

    if (tokens == NULL) {
        return;
    }

    int counter = 0;

    while(tokens[counter] != NULL){
        free(tokens[counter]);
        counter++;
        // free all tokens till null pointer is hit
    }

    free(tokens);
}

/* 
    Handles built-in commands ("exit" and "history") without forking.
   Parameters:
       arguments - argv-style array parsed from the user input.
       command   - original command line string (unused currently).
   Returns:
       1 if "exit" case
       0 if "history" case
       2 if the command was not a built-in. */

int Builtins(char **arguments, char *command) {

    //exit builtin
    if (strcmp(arguments[0], "exit") == 0) {
        freeTokens(arguments);
        return 1;
    }
    //history builtin
    if (strcmp(arguments[0], "history") == 0) {
        printHistory();
        freeTokens(arguments);
        return 0;
    }

    return 2; //if not those 2 commands
}


int main(){

    char command[MAX_CMD_LENGTH];
    // cannot use command from struct because that's for previous commands

    signal(SIGCHLD, SIG_IGN);// background child that exits gets cut automatically 

    while(1) {

        printf("catshell> ");

        fflush(stdout); // makes sure promt appears right away

        char* userInput = fgets(command, sizeof(command), stdin);
        //read in from the user

        if(userInput == NULL){
            break;
        }

        int stringLen = strlen(command);

        

        if(stringLen > 0 && command[stringLen - 1] == '\n'){
            command[stringLen - 1] = '\0';
        }
        //take out '\n' from command lines


        if(command[0] == '\0'){
            continue;
        }
        //if a line is blank, skip it 

        int background = 0; // define backgroud

        // !num cases first
        if (command[0] == '!') {

            int invalid = 0;

            if (command[1] == '\0') {
                // if there is no num, it's invalid
                invalid = 1;
            } 
            else {

                char *digits = command + 1; //point to everything after ! 

                int idx = 0;

                while(digits[idx] != '\0'){

                    if(!isdigit(digits[idx])){
                        invalid = 1;
                        break;
                    }
                    idx++;
                }

                if (!invalid) { // if all the digits are integers

                    int ID = atoi(digits); // get the ID from the input

                    if (ID < 1) {
                        invalid = 1;
                    } 
                    else {
                        // valid !num cases
                        history_t *num = NULL;
                        // find the commandID in history 
                        for (int i = 0; i < count; i++) {
                            int HistoryIdx = (head + i) % HISTORY;
                            if (history[HistoryIdx].commandID == ID) {
                                num = &history[HistoryIdx];
                                break;
                            }
                        }
                        
                        if(num == NULL){
                            invalid = 1;
                        }

                        else{

                            //make a new comand from the old command 
                            strncpy(command, num[0].command, MAX_CMD_LENGTH - 1);

                            command[MAX_CMD_LENGTH - 1] = '\0';

                        }
                    }
                }
            }

            if(invalid) {
                printf("%s: event not found\n", command);
                continue;
            }
        }

        // if it's not a !num case

        addToHistory(command); // add command to history right away 

        char **arguments = parseCommand(command, &background); // parse command (like python split())
        // gcc -Wall foo.c becomes ["gcc", "-Wall", "foo.c"]

        if(arguments == NULL || arguments[0] == NULL){
            // if parsing failed
            freeTokens(arguments);
            continue;
        }
        
       
        int bi = Builtins(arguments, command); // see if command is builtin function

        if(bi == 1){// exit case
            break;
        }
        if(bi == 0){ //history case
            continue;
        }
        if(bi == 2){// not a history case

            pid_t pid = fork();

            if(pid < 0){
                //failed to create child case
                perror("fork failure");
                freeTokens(arguments);
                continue;
            }
            if(pid == 0){
                //child case
                execvp(arguments[0], arguments);
                //execute new program
                printf("%s: command not found\n", arguments[0]);
                fflush(stdout);
                exit(1);
            }

            if(!background){ // if there is no background
                if(waitpid(pid, NULL, 0) < 0){
                    perror("waitpid failure");
                }
            }


        }

        freeTokens(arguments);
        
    }

    return 0; 
}