## <center> 实验二：添加Linux系统调用及熟悉常见系统调用 </center>

* ##### 实验目的

  * 学习如何添加Linux系统调用
  * 熟悉Linux下常见的系统调用

  

* ##### 实验环境

   * 主机：Ubuntu 32位 14.04

   * Linux内核版本：2.6.26

     

* ##### 实验内容

  * 添加Linux系统调用

    * > 需要添加的系统调用
      >
      > ``` c
      > void print_val(int_a);
      > //通过printk在控制台打印k的值
      > 
      > void str2num(char *str, int str_len, int *ret);
      > //将一个有str_len个数字的字符串str转换成十进制数字，然后将结果写到ret指向的地址中
      > 
      > ```

    * 分配系统调用号

      > linux-2.6.26/include/asm/unistd_32.h

    * 修改系统调用表

      > linux-2.6.26/arch/x86/kernel/syscall_table_32.S

    * 声明系统调用函数

      > linux-2.6.26/include/linux/syscalls.h

    * 在kernel/sys.c中实现新增的两个系统调用函数

      * sys_print_val(int a)

        ``` c
        asmlinkage long sys_print_val(int a)
        {
                printk("in sys_print_val:%d\n", a);
                return 0;
        }
        
        ```

      * sys_str2num(char \__user *str, int str_len, int __user *ret)

        ``` c
        asmlinkage long sys_str2num(char __user *str, int str_len, int __user *ret)
        {
                char kstr[100];
                int knum = 0;
                int i;
        
                if(copy_from_user(kstr, str, str_len))
                {
                        printk("copy from user is error\n");
                        return -EFAULT;
                }
        
                for(i = 0; i < str_len; i++)
                        knum = 10 * knum + (kstr[i] - '0');
        
                if(copy_to_user(ret, &knum, sizeof(knum)))
                {
                        printk("copy to user is error\n");
                        return -EFAULT;
                }
        
                return 0;
        }
        
        ```

    * 编译内核

    * 编写测试程序

      ``` c
      #include<stdio.h>
      #include<sys/syscall.h>
      #include<unistd.h>
      #include<string.h>
      
      int main()
      {
      	printf("Give me a string:\n");
      	char str[100];
      	int ret;
      	scanf("%s",str);
      	int len = strlen(str);
      	syscall(328,&str,len,&ret);
      	syscall(327,ret);
      	return 0;
      }
      
      ```

    * 重新生成根文件系统，运行qemu进入shell环境后执行测试程序

      

  * 熟悉常见的系统调用

    * 熟悉以下系统调用

      > ``` c
      > pid_t fork();
      > //创建进程
      > 
      > pid_t waitpid(pid_t pid,int* status,int options);
      > //等待指定pid的子进程结束
      > 
      > int execv(const char *path, char *const argv[]);
      > //根据指定的文件名或目录名找到可执行文件，并用它来取代原调用进程的数据段、代码段和堆栈段，在执行完之后，原调用进程的内容除了进程号外，其他全部被新程序的内容替换了
      > 
      > int system(const char* command);
      > //调用fork()产生子进程，在子进程执行参数command字符串所代表的命令，此命令执行完后随即返回原调用的进程
      > 
      > FILE* popen(const char* command, const char* mode);	//popen函数先执行fork，然后调用exec以执行command，并且根据mode的值（"r"或"w"）返回一个指向子进程的stdout或指向stdin的文件指针
      > 
      > int pclose(FILE* stream);
      > //关闭标准I/O流，等待命令执行结束
      > 
      > ```

    * 利用上面的系统调用函数实现一个简单的shell程序

      * 要求

        * n每行命令可能由若干个子命令组成，如"ls -l; cat 1.txt; ps -a"，由“;”分隔每个子命令，需要按照顺序依次执行这些子命令。

        * n每个子命令可能包含一个管道符号“|”，如"cat 1.txt | grep abcd"（这个命令的作用是打印出1.txt中包含abcd字符串的行），作用是先执行前一个命令，并将其标准输出传递给下一个命令，作为它的标准输入。

        * n最终的shell程序应该有类似如下的输出：

          ``` a
          OSLab2->echo abcd;date;uname -r
          abcd
          Mon Apr 8 09:35:57 CST 2019
          2.6.26
          OSLab2->cat 1.txt | grep abcd
          123abcd
          abcdefg
          
          ```

      * 实现

        ``` c
        #include <stdlib.h>
        #include <stdio.h>
        #include <unistd.h>
        #include <string.h>
        #include <readline/readline.h>
        #include <readline/history.h>
        
        #define MAXSIZE 64
        
        
        void do_argv(char **argv){
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
            int flag = 0;						//1为管道型指令，0为普通指令
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
                        buf[i] = '\0';
                        argv[++j] = &buf[i + 1];
                    }
                    i = i + 1;
                }
                argv[++j] = NULL;
                do_argv(argv);
            }
        
        }
        
        void do_cmd(char* buffer){
            char *p;
            char buf[64];
            strcpy(buf, buffer);
            int i = 0;
            int j = 0;
            p = buf;
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
        
        /* 无readlin库的普通指令读取 */
        //int main(){
        //
        //    char buffer[MAXSIZE];
        //	  char *find;
        //    while(1){
        //        printf("OSLab2->");
        //        fgets(buffer, MAXSIZE, stdin);
        //		  find = strchr(buffer, '\n');
        //        if(find) *find = '\0';
        //        do_cmd(buffer);
        //    }
        //}
        
        int main()
        {
            char* input, shell_prompt[100];
            char buffer[64];
            char *find;
        
            // Configure readline to auto-complete paths when the tab key is hit.
            rl_bind_key('\t', rl_complete);
        
            while(1) {
                // Create prompt string from user name and current working directory.
                snprintf(shell_prompt, sizeof(shell_prompt), "OSLab2->");
        
                // Display prompt and read input (n.b. input must be freed after use)...
                input = readline(shell_prompt);
        
                // Check for EOF.
                if (!input)
                    break;
        
                // Add input to history.
                add_history(input);
        
                // Do stuff...
        		do_cmd(input);
        
                // Free input.
                free(input);
            }
        }
        
        ```

* ##### 实验总结

  * 本次实验主要学习了如何添加Linux系统调用，熟悉了常见的系统调用。细节方面，学习了有：
    * 新增系统调用的方法；
    * 通过syscall函数执行系统调用的方法；
    * 常见系统调用函数的功能与参数；
    * shell运行的基本原理与简单的实现方式
  * 主要反思有：
    * 复习C语言文件读写和参数传递的规范；
    * 注意popen函数使用规范，如必须在结束后用pclose关闭I/O流

