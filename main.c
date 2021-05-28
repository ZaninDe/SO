#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_WORD 10
#define MAX_CHAR 100
#define DEL " "

int input_redirection_flag = 0; // flag de redirecionador
int output_redirection_flag = 0; // flag de redirecionador
int piping_flag = 0; // flag de pipe
char* input_file = NULL;
char* output_file = NULL;

void printDir() {
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  printf("\n%s > ", cwd);
}

int builtin(char* args[]) {
  int size = 2, argumentosIdentificador = 0;
  char* comandosEspecificos[size];

  comandosEspecificos[0] = "exit";
  comandosEspecificos[1] = "cd";

  for(int i = 0; i< size; i++) {
    if(strcmp(args[0], comandosEspecificos[i]) == 0) {
      argumentosIdentificador = i+1;
      break;
    }
  }

  switch (argumentosIdentificador)
  {
  case 1:
    exit(0);
    break;
  
  case 2:
    chdir(args[1]);
    break;

  default:
    break;
  }

  return 0;
}

void remove_end_of_line(char line[]) // troca o \n da string por \0
{ 
  int i = 0;

  while (line[i] != '\n')
    i++;

  line[i] = '\0';
}

int read_line(char line[])
{ // leitura da entrada
  char* ret = fgets(line, MAX_CHAR, stdin); // conteudo da linha

  remove_end_of_line(line);

  if(ret == NULL) // terminar execucao do shell
  exit(0);

  return 1;
}

int process_line(char *temp[], char line[]) // processa a linha de entrada
{
  int i = 0;
  temp[i] = strtok(line, DEL); // quebra a string por espaços vazios em tokens

  while (temp[i] != NULL)
  {
    i++;
    temp[i] = strtok(NULL, DEL); //  vai para o prox conteúdo da string
  }

  return 1;
}

char* sequence_operator_checking(char* aux[], char line[]) {
  int i = 0;
  int j;
  aux[i] = strtok(line, ";"); // quebra a string por ";" em tokens
  i++;

  while (aux[i] != NULL)
  {
    aux[i] = strtok(NULL, ";"); //  vai para o prox conteúdo da string após ";"
    i++;
  }

  return aux;
}

int pipe_and_redirection_checking(char* temp[]) { // verifica <, > e |
  int i = 0;

  while(temp[i] != NULL) {
    if(strcmp(temp[i],">") == 0) {
      output_redirection_flag = 1;
      output_file = temp[i+1]; // ENTENDER ESTA LINHA
      return i;
    }
    
    if(strcmp(temp[i],"<") == 0) {
      input_redirection_flag = 1;
      input_file = temp[i+1]; // ENTENDER ESTA LINHA
      return i;
    }

    if(strcmp(temp[i],"|") == 0) {
      piping_flag = 1;
      return i;
    }

    i++;
  }

  return i;
}

int check_line(char* temp[]) { // conta a quantidade de pipes ou redirections
  int i = 0;
  int pipe_cnt = 0;
  int output_redirection_cnt = 0;
  int input_redirection_cnt = 0;

  if (temp[i] == NULL) // caso a entrada seja nula
  {
    printf("No Command\n");
    return 1;
  }

  int total = 0;

  while(temp[i] != NULL) {
    if(strcmp(temp[i],">") == 0)
      output_redirection_cnt++;

    if(strcmp(temp[i],"<") == 0)
      input_redirection_cnt++;

    if(strcmp(temp[i],"|") == 0)
      pipe_cnt++;

    i++;
  }

  total = input_redirection_cnt+output_redirection_cnt+pipe_cnt; // ENTENDER ESTE CASO DO IF (não aceita mais de 1 operador (pipe ou redirect))

  if(total > 1) {
    printf("sorry, this shell dosent handle this case\n");
    temp[0] = NULL;
  }
}

int read_parse_line(char *args[], char line[], char* piping_args[]) // faz o parse da entrada
{ 
  char* temp[MAX_WORD];
  int pos;
  int i = 0;
  int j = 0;
	char* sequence;

 
////////////////////////////////// PAREI AQUI, PRECISA JOGAR PRA FORA O TESTE[] EM UM LACO NA MAIN

  process_line(temp, line);

  check_line(temp);
  pos = pipe_and_redirection_checking(temp);

  while(i < pos) {
    args[i] = temp[i];
    i++;
  }

  args[i] = NULL;  // DAQUI PRA BAIXO PIPING
  i++;

  if(piping_flag == 1) {
    int j = 0;
    while(temp[i] != NULL) {
      piping_args[j] = temp[i];
      i++;
      j++;
    }
  }
  return 1;
}

void piping_handle(char* args[], char* piping_args[], int pipefd[]) { // trata o caso de pipe

  int pid, i;

  pid = fork();

  if(pid == 0) {
    dup2(pipefd[1], 1);
    close(pipefd[0]); // o pai nao precisa do fim deste pipe
    execvp(args[0], args);
    perror(args[0]);
  } else {
    dup2(pipefd[0], 0);
    close(pipefd[1]); // o filho nao precisa do fim deste pipe
    execvp(piping_args[0], piping_args);
    perror(piping_args[0]);
  }
}

int main()
{
  char *args[MAX_WORD];
  char line[MAX_CHAR];
  char* piping_args[MAX_WORD];
  int i;
  int k = 1;


  int pipefd [2]; // processo a esquerda e a direita do pipe
  pipe(pipefd); // chama a function pipe com o array de pipe

    printDir();

  
	
  

  while (read_line(line))
  {
    
    char* teste[MAX_CHAR];
	  sequence_operator_checking(teste, line);
    

    int j;

    for(j = 0; j < 2; j++) {
    read_parse_line(args, teste[j], piping_args);

    builtin(args); // chama a função para executar comandos builtin caso exista


    printDir();
    
    pid_t pid = fork();

    if (pid == 0) // processo filho: executa o comando
    {
      if(input_redirection_flag == 1 && input_file != NULL) // ENTENDER ESSA PARTE DO REDIRECT
        dup2(open(input_file,O_RDWR|O_CREAT, 0777),0);

      if(output_redirection_flag == 1 && output_file != NULL) // ENTENDER ESSA PARTE DO REDIRECT
        dup2(open(output_file,O_RDWR|O_CREAT, 0777),1);

      if(piping_flag == 1) {// caso haja um pipe no comando
        piping_handle(args, piping_args, pipefd);
        exit(0);
      }
        execvp(args[0], args);
    }
    else if(pid > 0) // processo pai: aguarda o termino do filho
    {
      waitpid(pid, 0);
      input_redirection_flag = 0;
      output_redirection_flag = 0;
      piping_flag = 0;
      input_file = NULL;
      output_file = NULL;
    }
    else { // erro
      perror("fork()");
      return -1;
    }
  }
  }
  return 0;
}
