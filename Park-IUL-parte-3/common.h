/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Este Módulo não deverá ser alterado, e não precisa ser entregue
 ** Nome do Módulo: common.h
 ** Descrição/Explicação do Módulo:
 **     Definição das estruturas de dados comuns aos módulos servidor e cliente
 **
 ***************************************************************************************/
#ifndef __COMMON_H__
    #define __COMMON_H__

    #include "/home/so/utils/include/so_utils.h"
    #include <time.h>                   // Header para as funções time(), localtime(), strftime()
    #include <signal.h>                 // Header para as constantes SIG_* e as funções signal() e kill()
    #include <unistd.h>                 // Header para as funções alarm(), pause(), sleep(), fork(), exec*() e get*pid()
    #include <sys/wait.h>               // Header para a função wait()
    #include <sys/stat.h>               // Header para as constantes S_ISFIFO e as funções stat() e mkfifo()
    #include <sys/ipc.h>                // Header para as funções de IPC
    #include <sys/sem.h>                // Header para as funções de IPC de SEM
    #include <sys/msg.h>                // Header para as funções de IPC de MSQ
    #include <sys/shm.h>                // Header para as funções de IPC de SHM

    typedef struct {
        char matricula[10];             // Matrícula da viatura
        char pais[3];                   // País
        char categoria;                 // Categoria da viatura
        char nomeCondutor[80];          // Nome do condutor
    } Viatura;

    typedef struct {
        Viatura viatura;                // Viatura a estacionar
        int  pidCliente;                // PID do processo Cliente
        int  pidServidorDedicado;       // PID do processo Servidor Dedicado
    } Estacionamento;

    typedef struct {
        Viatura viatura;                // Viatura a estacionar
        char dataEntrada[17];           // Timestamp de Entrada do Cliente no parque, no formato AAAA-MM-DDTHHhmm
        char dataSaida[17];             // Timestamp de Saída do Cliente do parque, no formato AAAA-MM-DDTHHhmm
    } LogItem;

    typedef struct {
        long msgType;                   // Tipo da Mensagem
        struct {
            int status;                 // Status da mensagem
            Estacionamento est;         // Informação sobre o CheckIn
            char infoTarifa[100];       // Informação da Tarifa (proveniente da FACE)
        } msgData;                      // Dados da Mensagem
    } MsgContent;

    #define MAX_ESPERA  5               // Tempo máximo de espera por parte do Cliente

    #define FILE_LOGFILE           "estacionamentos.txt"    // Ficheiro para realizar os logs de estacionamento
    #define FILE_USERS             "/etc/passwd"            // Ficheiro onde validar os utilizadores do servidor Tigre
    #define INVALID                -1                       // Utilitário que significa valor inválido
    #define DISPONIVEL             -1                       // Utilitário que significa valor inválido
   
    #define KEY_FACE                0x0FACE                 // IPC Key do provider externo FACE (Federação Automóvel de Controlo de Estacionamentos)
    #define MSGTYPE_LOGIN           1   // MsgType do Servidor Principal
    #define SEM_MUTEX_INITIAL_VALUE 1   // Valor inicial dos semáforos do tipo MUTEX
    #define SEM_MUTEX_BD            0   // Índice do semáforo no Grupo de Semáforos, relativo ao MUTEX para controlar o acesso à BD
    #define SEM_MUTEX_LOGFILE       1   // Índice do semáforo no Grupo de Semáforos, relativo ao MUTEX para controlar o acesso ao ficheiro FILE_LOGFILE
    #define SEM_SRV_DEDICADOS       2   // Índice do semáforo no Grupo de Semáforos, relativo à espera do fim dos Servidores Dedicados
    #define SEM_LUGARES_PARQUE      3   // Índice do semáforo no Grupo de Semáforos, relativo ao número de lugares do parque
    #define SEM_MUTEX_FACE          0   // Índice do semáforo no Grupo de Semáforos FACE, relativo ao MUTEX para controlar o acesso à SHM com a variável tarifaAtual
    #define DONT_CARE               0   // Utilitário
    #define IPC_GET                 0   // Utilitário
    #define ACCESS_PERMS         0600   // Permissões para a criação de elementos IPC
    #define NEW_ADDRESS          NULL   // Utilitário
    #define NO_FLAGS                0   // Utilitário
    #define SHMAT_ERROR   (void *) -1   // Utilitário
	#define SEM_DOWN               -1   // Utilitário
	#define SEM_UP                  1   // Utilitário
    #define _PROCESSO_FILHO         0   // Utilitário

    #define CLIENT_ACCEPTED             0   // Status correspondente a uma mensagem indicando que o Cliente foi aceite no parque
    #define ESTACIONAMENTO_TERMINADO    1   // Status correspondente a uma mensagem indicando que o Cliente NÃO foi aceite no parque ou que deve sair do parque
    #define INFO_TARIFA                 2   // Status correspondente a uma mensagem indicando qual é a tarifa (preço) atual do parque (atualizada ao minuto)
    #define TERMINA_ESTACIONAMENTO      3   // Status correspondente a uma mensagem indicando que o Cliente deseja terminar o estacionamento no parque

    /* Protótipos de funções */
    void s1_IniciaServidor(int argc, char *argv[]);
    void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque);
    void s1_2_ArmaSinaisServidor();
    void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId);
    void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId);
    void s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento);
    void s2_MainServidor();
    void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest);
    void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados);
    void s3_TrataCtrlC(int sinalRecebido);
    void s4_EncerraServidor();
    void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque);
    void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados);
    void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId);
    void s5_TrataTerminouServidorDedicado(int sinalRecebido);
    void sd7_MainServidorDedicado();
    void sd7_1_ArmaSinaisServidorDedicado();
    void sd7_2_ValidaPidCliente(MsgContent clientRequest);
    void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE);
    void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE);
    void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD);
    void sd8_1_ValidaMatricula(MsgContent clientRequest);
    void sd8_2_ValidaPais(MsgContent clientRequest);
    void sd8_3_ValidaCategoria(MsgContent clientRequest);
    void sd8_4_ValidaNomeCondutor(MsgContent clientRequest);
    void sd9_1_AdormeceTempoRandom();
    void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest);
    void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem);
    void sd10_1_AguardaCheckout(int msgId);
    void sd10_1_1_TrataAlarme(int sinalRecebido);
    void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem);
    void sd11_EncerraServidorDedicado();
    void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD);
    void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest);
    void sd12_TrataSigusr2(int sinalRecebido);
    
    void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId);
    void c1_2_ArmaSinaisCliente();
    void c2_1_InputEstacionamento(MsgContent *pclientRequest);
    void c2_2_EscrevePedido(int msgId, MsgContent clientRequest);
    void c3_ProgramaAlarme(int segundos);
    void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest);
    void c4_2_DesligaAlarme();
    void c5_MainCliente(int msgId, MsgContent *pclientRequest);
    void c6_TrataCtrlC(int sinalRecebido);
    void c7_TrataAlarme(int sinalRecebido);
#endif  // __COMMON_H__