/*
 copie os .lib e .a para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\lib\
 copie os .dll para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\bin\
 copie os .h para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\include\
 project -> build options -> linker settings -> Other linker options: inclua -lpthreadGC1
*/
#define MWIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#ifdef MWIN
#include <windows.h>
#endif
#define ERRO 0
#define OK   1

#define TEMPO_SLEEP 10

#define NUM 1000

// Variaveis globais

// declaracao dos mutexes
pthread_mutex_t G_p_fi;  // independente
pthread_mutex_t G_p_be;  // segundo - nao usado com G_p_bs
pthread_mutex_t G_p_bs;  // segundo - nao usado com G_p_be

// controladores dos buffers
int G_qtd_be;
int G_qtd_bs;

// variavel de controle do encerramento
int G_terminou;

// buffers de processamento
char G_be[6];
char G_bs[6];

void m_usleep(unsigned long pause)
{
#ifdef MWIN
   Sleep(pause);
#else
   usleep(pause*1000l);
#endif

   return;
}

void init_lock(pthread_mutex_t *lock) // inicializa as variáveis de lock, fazer isto antes do inicio das threads
{
   pthread_mutex_init(lock,NULL);
   return;
}

void fini_lock(pthread_mutex_t *lock) // finalize as variaveis de lock, apos o pthread_kill
{
   pthread_mutex_destroy(lock);
   return;
}

int gerar_entrada()
{
    FILE *arq;
    int i;
    if ((arq = fopen("e.txt","wt"))==NULL)
    {
        printf("\nERRO: criando o arquivo de entrada (e.txt)\n");
        return(ERRO);
    }

    for (i = 1 ; i <= NUM; ++i)
    {
        fprintf(arq,"%05d\n",i);
    }
    fflush(arq);
    printf("\nArquivo de entrada gerado\n\n");
    fclose(arq);

    return(OK);
}

void *escrita()
{
    int acabou = 0;
    FILE *arq;

    printf("Entrei escrita\n");
    arq = fopen("s.txt","wt");
    while (1)
    {
        if (!acabou)
        {
            /*
            Loop interno que fica verificando se há dados no buffer de saída.
            Trava o mutex do buffer de saída logo no início de cada loop, e destrava no final dos mesmos.
            Caso o buffer de saída esteja cheio, então escrevo o dado do buffer para o arquivo,
            limpa o buffer, e indica que agora ele está vazio.
            Caso esteja vazio, verifica se ainda há dados para serem processados.
            Se não houverem mais dados para serem processados, então a thread passa a variável
            de controle de encerramento para 0, indicando que a escrita acabou, e sai do loop interno
            ficando apenas no loop externo esperando a finalização da thread.
            */
            while(1){
                pthread_mutex_lock(&G_p_bs);
                if(G_qtd_bs==1)
                {
                    //printf("saiu = %s",G_bs);
                    fprintf(arq,"%s",G_bs);
                    fflush(arq);
                    memset((void *)G_bs,0,sizeof(char)*6);
                    G_qtd_bs = 0;
                }
                else
                {
                    if(G_terminou==1)
                    {
                        printf("Encerrei escrita\n");
                        acabou = 1;
                        G_terminou = 0;
                        fclose(arq);
                        break;
                    }
                }
            pthread_mutex_unlock(&G_p_bs);
            }
        }
        m_usleep(TEMPO_SLEEP);
   }
    return(NULL);
}

void *leitura()
{
    int i = 0;
    int acabou = 0;
    FILE *arq;
    printf("Entrei leitura\n");
    arq = fopen("e.txt","rt");
    fflush(stdout);
    while (1)
    {
        if (!acabou)
        {
            /*
            Loop interno que fica verificando se existem dados para serem lidos no arquivo.
            Trava o mutex do buffer de entrada logo no inicio dos loops, e destrava no final destes.
            Le o buffer estiver vazio, entao verifica se o arquivo ainda tem dados.
            Caso tenha, lê para o buffer, e passa a variável de controle do buffer para 1.
            Caso não tenha mais dados no arquivo de entrada, então passa a variável de controle
            de encerramento para 2, indicando que acabou a leitura, e sai do loop interno,
            ficando apenas no loop externo esperando a finalização da thread.
            */
            while(1){
                pthread_mutex_lock(&G_p_be);
                if(G_qtd_be==0){
                    //fflush(stdin);
                    if(!feof(arq)){
                        fscanf(arq,"%s",G_be);
                        G_qtd_be = 1;
                        //printf("entrou = %s\n",G_be);
                    }
                    else
                    {
                        pthread_mutex_lock(&G_p_fi);
                        printf("\nEncerrei leitura\n");
                        acabou = 1;
                        G_terminou = 2;
                        fclose(arq);
                        pthread_mutex_unlock(&G_p_fi);
                        pthread_mutex_unlock(&G_p_be);
                        break;
                    }
                }
                pthread_mutex_unlock(&G_p_be);
            }
        }
        m_usleep(TEMPO_SLEEP);
   }
   return(NULL);
}

void *processamento()
{
    int acabou = 0;
    int i;
    char aux[6];
    char var;
    memset(aux,0,sizeof(char)*6);

    printf("Entrei processamento\n");
    while (1)
    {
        if (!acabou)
        {
            /*
            Dois loops internos, onde fica verificando se o buffer de saída está vazio
            e outro se o buffer de entrada está cheio.
            Os mutexes dos buffers são travados antes de cada verificação e destravados após as mesmas.

            No loop do buffer de entrada, caso o buffer esteja cheio, transfere os dados do buffer para
            um buffer auxiliar, zera o buffer de entrada, indica que este agora esta vazio, destrava o mutex
            e para o loop interno do buffer de entrada. Caso o buffer de entrada esteja vazio, verifica se ainda
            existem dados para serem lidos do arquivo de entrada. Caso não, então finaliza o processamento,
            e passa a variavel de controle de encerramento para 2.

            No loop do buffer de saída, caso o buffer esteja vazio, inverte os dados do buffer auxiliar
            para o buffer de saída, indica que este agora está cheio, e para o loop interno do buffer de saída.

            */
            while(1)                                /// loop buffer de entrada
            {
                pthread_mutex_lock(&G_p_be);
                if(G_qtd_be==1)
                {
                    for(i=0;i<5;i++)
                    {
                        aux[i]=G_be[i];
                    }
                    memset((void *)G_be,0,sizeof(char)*6);
                    G_qtd_be = 0;
                    pthread_mutex_unlock(&G_p_be);
                    break;
                }
                else
                {
                    if(G_terminou==2)
                    {
                        pthread_mutex_lock(&G_p_fi);
                        printf("Encerrei processamento\n");
                        acabou = 1;
                        G_terminou = 1;
                        pthread_mutex_unlock(&G_p_fi);
                        break;
                    }
                }
                pthread_mutex_unlock(&G_p_be);
                //m_usleep(TEMPO_SLEEP);
            }
            while(1)                                ///loop buffer de saída
            {
                pthread_mutex_lock(&G_p_bs);
                if(G_qtd_bs==0)
                {
                    for(i=0;i<5;i++)
                    {
                        G_bs[i]=aux[4-i];
                    }
                    G_bs[5]='\n';
                    G_qtd_bs=1;
                    memset((void *)aux,0,sizeof(char)*6);
                    pthread_mutex_unlock(&G_p_bs);
                    break;
                }
                pthread_mutex_unlock(&G_p_bs);
            }
        }
        m_usleep(TEMPO_SLEEP);
    }
    return(NULL);
}

void* finalizar()
{
    int nao_acabou = 1;
    while (nao_acabou)
    {
        m_usleep(50);
        pthread_mutex_lock(&G_p_fi);
        if (G_terminou == 0)
        {
            printf("\nEm finalizar... Acabou mesmo!\n\n");
            nao_acabou = 0;
        }
        pthread_mutex_unlock(&G_p_fi);
    }
    return NULL;
}

int main(void)
{
    // declaração das pthreads
    pthread_t escrita_t, leitura_t, processamento_t;

    // inicializacao do G_terminou
    G_terminou = 3;

    // inicializacao dos mutexes de lock
    init_lock(&G_p_fi);
    init_lock(&G_p_be);
    init_lock(&G_p_bs);

    // limpeza dos buffers
    memset((void *) G_be,0,sizeof(char)*6);
    memset((void *) G_bs,0,sizeof(char)*6);

    // inicializacao dos controladores dos buffers
    G_qtd_be = 0;
    G_qtd_bs = 0;

    // geracao do arquivo de entrada
    if (!gerar_entrada())
    {
        printf("\nVou sair");
        return(1);
    }

    // chamada das pthreads
    if(pthread_create(&leitura_t,NULL,leitura,NULL)){
        printf("\nErro ao criar thread de leitura");
        return(1);
    }
    if(pthread_create(&processamento_t,NULL,processamento,NULL)){
        printf("\nErro ao criar thread de processamento");
        return(1);
    }
    if(pthread_create(&escrita_t,NULL,escrita,NULL)){
        printf("\nErro ao criar thread de escrita");
        return(1);
    }

    // Aguarda finalizar
    finalizar();

    // Matar as pthreads
    pthread_kill(leitura_t,1);
    pthread_kill(processamento_t,1);
    pthread_kill(escrita_t,1);

    // Finalização dos mutexes
    fini_lock(&G_p_fi);
    fini_lock(&G_p_be);
    fini_lock(&G_p_bs);

    return(0);
}
