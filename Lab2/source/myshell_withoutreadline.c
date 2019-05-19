#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MAXSIZE 64


void do_argv(char **argv){
//    int i;
//    while(argv[i] != NULL){
//	printf("%s\n",argv[i]);
//	i = i + 1;
//    }
    pid_t pid = fork();
    if(pid == 0){
        execvp(argv[0], argv);
    }
    else{
        int status;
        status = waitpid(pid, &status, 0);
    }
}

void do_pipe(char *left, char *right){
    char buf[64];
//    printf("left pipe:%s\n",left);
//    printf("right pipe:%s\n",right);
    FILE *fr = popen(left, "r");
    FILE *fw = popen(right, "w");
    fread(buf, 1, 64, fr);
    fwrite(buf, 1, 64, fw);
    pclose(fr);
    pclose(fw);
}

void do_cut(char* p){
    char buf[64];
    strcpy(buf, p);
    int i = 0;
    char *left;
    char *right;
    int flag = 0;
    while(buf[i] != '\0'){
        if(buf[i] == '|'){
            flag = 1;
            buf[i - 1] = '\0';
            left = buf;
            right = &buf[i + 2];
            do_pipe(left, right);
        }
        i = i + 1;
    }
    if(!flag){
//        system(p);
        char* argv[5];
        i = 0;
        int j = 0;
        argv[0] = buf;
        while(buf[i] != '\0'){
            if(buf[i] == ' '){
		j = j + 1;
                buf[i] = '\0';
                argv[j] = &buf[i + 1];
            }
            i = i + 1;
        }
        argv[j + 1] = NULL;
        do_argv(argv);
    }

}
void do_cmd(char* buffer){
    char *p;
    char buf[64];
    strcpy(buf, buffer);
    int i = 0;
    int j = 0;
    p = &buf[0];
    while(buf[i] != '\0'){
        if(buf[i] == ';') {
            buf[i] = '\0';
            do_cut(p);
            p = &buf[i + 1];
        }
        i = i + 1;
    }
    do_cut(p);
}

int main(){

    char buffer[MAXSIZE];
    char *find;
    while(1){
        printf("OSLab2->");
        fgets(buffer, MAXSIZE, stdin);
	find = strchr(buffer, '\n');
	if(find) *find = '\0';
//        strcpy(buffer, "date;ls;echo abcd | cat 1.txt");
        do_cmd(buffer);
    }
}
