// ex3 - Lior Zadah - 318162930
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define WORDS_LENGTH 512

void execPipe(char *command1[], char *command2[]);
void exec2Pipe(char *command1[], char *command2[], char *command3[]);
char **creatCommand(char *sentence);
void printToHistory(char *sentence);
void findInHistory(char *sentence, int *flag);
void deleteSpace(char *string);
int pipeHendelr(char *sentence);
void handler(int num);
void seperateSentence ( char* sentence, char * oneCommand, char * twoCommand);

int totalCommands = 0, totalPipes = 0, totalWordsFinal = 0, flag = 0; // for the done comment // for the done words //for the '!' part to know if we found the line we needed
int pipeIndex1 = 0;

int main() {
    char sentence[WORDS_LENGTH];
    char **command; // the commands array :)
    char cwd[WORDS_LENGTH]; //need it to print cwd later instead of the sentence
    int amp = 0; // flag for as to know if we have '&' in the sentence
    FILE *fp;
    pid_t x;
    // if we have & at the end of the word it will catch the signal and use hendler func to wait by pid
    // so the shell could still operate and do the command in the background
    signal(SIGCHLD, handler);
    while (1) {
        getcwd(cwd, sizeof(cwd)); // to print the user the cwd in every iteration
        printf("%s>", cwd);
        fgets(sentence, 510, stdin);
        sentence[strlen(sentence) - 1] = '\0';      // to remove the last char that is \n
        // if we have a command whit ' ' at first or last, add command and continue
        if (sentence[0] == ' ' || sentence[strlen(sentence) - 1] == ' ') {
            printf("Invalid syntax\n");
            continue;
        }
        /*
         * For the pipe part we will start with a check to know how many pipes we have in the sentence.
         * Then, if we have we will operate the func 'pipeHandler',
         * and she will also tell as if we have '!' in the text and return -1 if we insert invalid num.
         * If every thing went well, add to total commands and print to history.
         */
        int pipeTemp = 0;
        for (int i = 0; i < strlen(sentence); i++)
            if (sentence[i] == '|')
                pipeTemp++;
        if (pipeTemp > 0) { // if we have pipes????--------------------
            int a = pipeHendelr(sentence);
            if (a == -1) { // here we are returning -1 if in one of the pipes we got invalid number
                perror("Number didn't found in history");
                continue;
            }
            continue;
        }

        if (strcmp(sentence, "done") == 0) { // if the user press done - print words & commands num and exit
            totalCommands++;
            printf("Total number of commands: %d\n", totalCommands);
            printf("Total number of words: %d\n", totalWordsFinal);
            //printf("Total number of pipes: %d\n", totalPipes);
            printf("See you next time!\n");
            break;
        }
        if (sentence[0] == 'c' && sentence[1] == 'd' && (sentence[2] == ' ' || sentence[2] == '\0')) {
            printf("command not supported (Yet)\n");
            totalCommands++, totalWordsFinal++;
            continue;
        }

        // check if the user insert ! and want to recall func whit number
        if (sentence[0] == '!' && pipeTemp == 0) { // if the user want to activate an order by their number hi just need to insert ! before the line num.
            flag = 0;
            findInHistory(sentence, &flag);
            if (flag == 0) {// need  to check if the number is not in the history
                printf("Not in history\n");
                continue;
            }// check if we have pipes in the command with '!'
            for(int i = 0; i < strlen(sentence); i++){
                if(sentence[i] == '|'){
                    pipeHendelr(sentence);
                    break;
                }
            }
            continue;
        }
        // in history we're: open the file, print all the priviuse commands, close the file and add the command history to the file.
        if (strcmp(sentence, "history") == 0) {
            totalCommands++;
            fp = fopen("file.txt", "a");// open to append the word history to the file
            if (fp == NULL) {
                perror("cannot open file\n");
                exit(0);
            }
            fprintf(fp, "%s", sentence); // print to file the string whit \n
            fprintf(fp, "\n");
            fclose(fp);//close file

            fp = fopen("file.txt", "r");// open the file to read, if it is failed, print
            if (fp == NULL) {
                perror("cannot open file\n");
                exit(0);
            }
            char str[WORDS_LENGTH];
            int lineNum = 1;
            while (fgets(str, WORDS_LENGTH, fp) != NULL) {//print every sentence whit line number
                printf("%d: %s", lineNum, str);
                lineNum++;
            }
            fclose(fp); //closing the file :)
            continue;
        }
        // ex3 - the second part.
        amp = 0; // restart the int
        if (sentence[strlen(sentence) - 1] == '&') {
            amp = 1; // update flag and remove &
            sentence[strlen(sentence) -1 ] = '\0';
        }

        command = creatCommand(sentence);
        ////only one command
        if(strcmp(command[0], "nohup") == 0) {
            printToHistory(sentence);
            x = fork(); // making anoder process
            if (x == 0) {
                close(STDIN_FILENO);
                int fd = open("nohup.txt", O_WRONLY | O_CREAT | O_APPEND,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                int val = dup2(fd, STDOUT_FILENO);
                if (val == -1) {
                    fprintf(stderr, "dup2 FAIL\n");
                    exit(1);
                }
                signal(SIGHUP, SIG_IGN);
                if ((execvp(command[1], command + 1)) == -1) {//check if the command didn't work
                    perror("execvp() failed");
                    exit(1);
                }
            }
            totalCommands++;
            for (int i = 0; command[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command[i]);
            free(command);
            continue;
        }
        printToHistory(sentence);
        x = fork(); // making anoder process
        if (x == 0) {
            // getting the command done.
            if ((execvp(command[0], command)) == -1) {//check if the command didn't work
                perror("execvp() failed");                   // print error.
                totalCommands++;
                exit(1);
            }
        }
        if(amp == 0) {
            pause(); // wait for the son if the're isn't &
        }
        for (int i = 0; command[i] != NULL; i++) //releasing the malloc that we used by free.
            free(command[i]);
        free(command);
    }
}


void execPipe(char *command1[], char *command2[]) {
    /*
     * here we are going to execvp two commands, the STDOUT of the first wil go to the STDIN of the second
     * it will be his input.
     * while opening the dop2 we also check if he fail or not.
     */
    int fds[2]; // open array for the pipe close & open stdin & out
    int p = pipe(fds);// create pipe: fds[0] is the input and fds[1] is the output
    if (p == -1) {
        perror("PIPE failed");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork(); // macking child process
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // in the child proccess
        int d = dup2(fds[1], STDOUT_FILENO); //duplicate the pipe to stdin (take input from the pipe to stdin)
        if(d == -1){
            perror("dup2 has failed");
            exit(1);
        }
        close(fds[1]); //close the file descriptors - stdin & out
        close(fds[0]);
        if ((execvp(command1[0], command1)) == -1) {//check if the command didn't work
            perror("execvp() failed");                   // print error.
            for (int i = 0; command1[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command1[i]);
            free(command1);
            exit(1);
        }
    }
    pid_t pid2 = fork(); // using only child process.
    if (pid2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) { // in the child proccess
        int d = dup2(fds[0], STDIN_FILENO); //duplicate the output to the pipe (send stdout output to the pipe)
        if(d == -1){
            perror("dup2 has failed");
            exit(1);
        }
        close(fds[0]); //close the file descriptors
        close(fds[1]);
        if ((execvp(command2[0], command2)) == -1) {//check if the command didn't work
            perror("execvp() failed");                   // print error.
            for (int i = 0; command2[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command2[i]);
            free(command2);
            exit(1);
        }
    }
    for (int i = 0; command1[i] != NULL; i++) //releasing the malloc that we used by free for the 1st command.
        free(command1[i]);
    free(command1);
    for (int i = 0; command2[i] != NULL; i++) //releasing the malloc that we used by free  for the 2nd command.
        free(command2[i]);
    free(command2);
    close(fds[0]); //close the file descriptors
    close(fds[1]);
    wait(NULL);
    wait(NULL);
}

void exec2Pipe(char *command1[], char *command2[], char *command3[]) {
    /*
     * Here we're gonna use 2 fds array's and 3 dup2 becouse we have 3 commands to handel.
     * we need to pass the stdout of the 1st command to the stdin of the 2nd command'
     * and the stdin of the 3rd command from the 2nd :)
     * making 3 fork to ecevp the command properly.
     * of cours we check every time if the command / dup2 / fork failed we return perror.
     */
    int fds[2];
    int fds2[2];
    int p1 = pipe(fds);// create pipe: fds[0] is the input and fds[1] is the output
    int p2 = pipe(fds2);
    if (p1 == -1 || p2 == -1) {
        perror("PIPE failed");
        exit(EXIT_FAILURE);
    }
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork1 failed");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) { // in the child proccess
        int d = dup2(fds[1], STDOUT_FILENO); //duplicate the pipe to stdin (take input from the pipe to stdin)
        if(d == -1){
            perror("dup2 has failed");
            exit(1);
        }
        close(fds[1]); //close the file descriptors
        close(fds[0]);
        close(fds2[1]); //close the file descriptors
        close(fds2[0]);
        if ((execvp(command1[0], command1)) == -1) {//check if the command didn't work
            perror("execvp() failed");                   // print error.
            for (int i = 0; command1[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command1[i]);
            free(command1);
            exit(1);
        }
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork2 failed");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) { // in the child proccess
        int d = dup2(fds[0], STDIN_FILENO); //duplicate the output to the pipe (send stdout output to the pipe)
        int t = dup2(fds2[1], STDOUT_FILENO);
        if(d == -1 || t == -1){
            perror("dup2 has failed");
            exit(1);
        }
        close(fds[1]); //close the file descriptors
        close(fds[0]);
        close(fds2[0]); //close the file descriptors
        close(fds2[1]);
        if ((execvp(command2[0], command2)) == -1) {//check if the command didn't work
            perror("execvp() failed");                   // print error.
            for (int i = 0; command2[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command2[i]);
            free(command2);
            exit(1);
        }
    }

    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("fork3 failed");
        exit(EXIT_FAILURE);
    }
    if (pid3 == 0) { // in the child proccess
        int d = dup2(fds2[0], STDIN_FILENO); //duplicate the output to the pipe (send stdout output to the pipe)
        if(d == -1){
            perror("dup2 has failed");
            exit(1);
        }
        close(fds[1]); //close the file descriptors
        close(fds[0]);
        close(fds2[0]); //close the file descriptors
        close(fds2[1]);
        if ((execvp(command3[0], command3)) == -1) {//check if the command didn't work
            perror("execvp() failed");                    // print error.
            for (int i = 0; command3[i] != NULL; i++) //releasing the malloc that we used by free.
                free(command3[i]);
            free(command3);
            exit(1);
        }
    }
    for (int i = 0; command1[i] != NULL; i++) //releasing the malloc that we used by free.
        free(command1[i]);
    free(command1);
    for (int i = 0; command2[i] != NULL; i++) //releasing the malloc that we used by free.
        free(command2[i]);
    free(command2);
    for (int i = 0; command3[i] != NULL; i++) //releasing the malloc that we used by free.
        free(command3[i]);
    free(command3);
    close(fds[0]); //close the file descriptors
    close(fds[1]);
    close(fds2[0]); //close the file descriptors
    close(fds2[1]);
    wait(NULL);//wait for the father of every process.
    wait(NULL);
    wait(NULL);
}


char **creatCommand(char *sentence) {
    /*
     * Here we're gonna make the command, like the 2nd project we are counting words and chars to
     * make the argv array whit index for every word and at the last place theris NULL.
     */
    char **command;
    int wordCount = 0;
    int charCount = 0;
    for (int i = 0; sentence[i] != '\0'; i++) {  // count the words at the sentence to apply malloc
        if (sentence[i] != ' ') {
            while (sentence[i] != ' ' && sentence[i] != '\0')
                i++;
            wordCount++;
        }
    }
    totalWordsFinal += wordCount;
    command = (char **) malloc((wordCount + 1) * sizeof(char *));// malloc by the amount of words
    if (command == NULL) {
        perror("error: malloc fail");
        exit(0);
    }
    char temp[WORDS_LENGTH] = "-";
    wordCount = 0; // reset counter
    for (int i = 0; sentence[i] != '\0'; i++) {
        if (sentence[i] != ' ') {
            while (sentence[i] != ' ' && sentence[i] != '\0') {
                temp[charCount] = sentence[i]; // insert the char to the string by order.
                temp[++charCount] = '\0';// to insert '\0' at the end of the word. we don't want garbage value in our string....
                i++;
            }// need to add anoder check if the malloc didnt was an sucsess
            // need to copy to the array the word[i] and open length by the length of the word.
            command[wordCount] = (char *) malloc((charCount + 1) * sizeof(char));
            if (command[wordCount] == NULL) {
                perror("error: malloc fail");
                free(command);
                exit(0);
            }
            strcpy(command[wordCount], temp);//insert the string to his place in the array
            strcpy(temp, "-");              // to reset temp to be able to receive new word.
            wordCount++;                             //new word
        }
        charCount = 0;// reset char count for every word
    }
    command[wordCount] = NULL;
    return command;
}


void printToHistory(char *sentence) {
    /*
     * open FILE pointer and write the sentence to the history.
     */
    FILE *fp;
    fp = fopen("file.txt", "a");
    if (fp == NULL) {
        perror("cannot open file\n");
        exit(0);
    }
    fprintf(fp, "%s", sentence); // print to file the string whit \n
    fprintf(fp, "\n");
    fclose(fp);//close file
}

void findInHistory(char *sentence, int *flag) {
    /*
     * here we are getting a number of line in the history and look for the command in the file.
     * we are reciving a number and a flag to tell us if the line was founded or not.
     * in the end we will replace the number whit the command from the file.
     */
    FILE *fp;
    for (int i = 1; i < strlen(sentence); ++i) {// if he inserts ! with non-digits - STOP!
        if (sentence[i] < '0' || sentence[i] > '9')
            break;
    }
    sentence[0] = '0'; // replace the ! with '0' so the atoi func will work.
    int neededLine = atoi(sentence); // it will give us back the line number
    int counter = 1;
    char buffer[WORDS_LENGTH + 1];

    fp = fopen("file.txt", "r");// open to append the word history to the file
    if (fp == NULL) {
        perror("cannot open file\n");
        exit(0);
    }
    while (fgets(buffer, WORDS_LENGTH,fp)) { // reading from the history file and comper the number the user gaive us with the counter we update in evry line.
        if (neededLine == counter) {// if the line num == counter - we have found the command we want to apply.
            strcpy(sentence, buffer); // replace the sentence with the new command to be done by the rest of the code.
            sentence[strlen(sentence) - 1] = '\0';
            *flag = 1;
            break;
        }
        counter++;
    }
    fclose(fp);
}

void deleteSpace(char *string) {
    /*
     * simple func to delete the space in the beginning of the command.
     * we need this func when we have pipes whit '!' and we want to know if in the first place in the sentence we have !
     * but some times we have space between the | and the number
     * so this func is here to help.
     */
    int index = 0, i, j;
    // Find last index of ' ' char
    while (string[index] == ' ' || string[index] == '\t' || string[index] == '\n')
        index++;

    if (index != 0) { //return the sentence without the unnecessary spaces.
        i = 0;
        while (string[i + index] != '\0') {
            string[i] = string[i + index];
            i++;
        }
        string[i] = '\0'; // Make sure that string is NULL terminated
    }
}

int pipeHendelr(char *sentence) {
    /*
     * the point of the func is to be the "master of all pipes"
     * when we detect we have pipes we call this func and she is opening the rigth things and call the other funcs.
     * start with seperete the sentens to the first command and the other sentence.
     * if we have one more pipe in the second part we seperat it too so were gonna get 3 char ** whit the commands
     * and call to the final funcs to execvp them.
     */
    char oneCommand[WORDS_LENGTH] = "-";
    char twoCommand[WORDS_LENGTH] = "-";
    char threeCommand[WORDS_LENGTH] = "-";
    int toManyPipe = 1;
    totalCommands++;
    printToHistory(sentence);
    seperateSentence(sentence, oneCommand, twoCommand);
    deleteSpace(twoCommand);// if we need to recover command from the history, delete spaces and find it.
    if (oneCommand[0] == '!') {
        flag = 0;
        findInHistory(oneCommand, &flag);
        if (flag == 0) {// need  to check if the number is not in the history
            printf("The number in the first command is not in history\n");
            return -1;
        }

        for (int i = 0; i < strlen(oneCommand); i++) { // if we have anuder pipe in the new command handel it.
            if (oneCommand[i] == '|') {
                toManyPipe++;
                strcpy(threeCommand, twoCommand);
                strcpy(sentence, oneCommand);
                seperateSentence(sentence, oneCommand, twoCommand);
                break;
            }
        }
    }
    //once more for the second command.
    deleteSpace(twoCommand); // if we have space before the '!'
    if (twoCommand[0] == '!') {
        flag = 0;
        findInHistory(twoCommand, &flag);
        if (flag == 0) {// need  to check if the number is not in the history
            printf("The number in the second command is not in history\n");
            return -1;
        }
        for(int i = 0; i < strlen(twoCommand); i++){
            if(twoCommand[i] == '|'){
                toManyPipe++;
                strcpy(sentence, twoCommand);
                seperateSentence(sentence, twoCommand, threeCommand);
                break;
            }
        }
    }
    //now we are doing it to make sure we have one command in the second place
    //if not seperte them and continue
    for(int i = 0; i < strlen(twoCommand); i++){
        if(twoCommand[i] == '|'){
            toManyPipe++;
            strcpy(sentence, twoCommand);
            seperateSentence(sentence, twoCommand, threeCommand);
            deleteSpace(threeCommand);// if we have space before the '!'
            break;
        }
    }
    /*
     * now, if everting went well make the sentences commands and send them to the execvp :)
     */
    if (toManyPipe == 1) { // if  there is only one pipe
        char **firstCommand = creatCommand(oneCommand);
        char **secondCommand = creatCommand(twoCommand);
        totalPipes++;
        execPipe(firstCommand, secondCommand);
    } else if (toManyPipe == 2){
        if(strcmp(twoCommand, "sort") == 0){ //insert space after sort - bug fixing :)
            twoCommand[4] = ' ';
            twoCommand[5] = '\0';
        }
        char **firstCommand = creatCommand(oneCommand);
        char **secondCommand = creatCommand(twoCommand);
        char **thirdCommand = creatCommand(threeCommand);
        totalPipes += 2;
        exec2Pipe(firstCommand, secondCommand, thirdCommand);
    }
        //if we have pipes > 2 - return error.
    else {
        fprintf(stderr, "TO MANY PIPES!");
    }
    return 0;
}

void handler(int num){
    //this func will handel the '&' part - like Tamar showed us.
    waitpid(-1, NULL, WNOHANG);
}

void seperateSentence ( char* sentence, char * oneCommand, char * twoCommand){
    /*
     * Here we are finding the index of the '|', and divied them to 2 char*,
     */
    pipeIndex1 = 0;
    for (int i = 0; i < strlen(sentence); i++) {//check if we have pipe!1
        if (sentence[i] == '|') {
            pipeIndex1 = i;
            break;
        }
    }
    int second = 0; // the index for the 2nd command.
    for (int i = 0; i < strlen(sentence); i++) {
        if (i < pipeIndex1) {
            oneCommand[i] = sentence[i];
            oneCommand[i + 1] = '\0'; // cutting the word becuose we dont whant garbig in the command.
        }
        if (i > pipeIndex1) {
            twoCommand[second] = sentence[i];
            twoCommand[++second] = '\0';
        }
    }
}