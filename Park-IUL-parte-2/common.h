/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
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
 #include <fcntl.h>
 #include <time.h>               // Header para as funções time(), localtime(), strftime()
 #include <signal.h>             // Header para as constantes SIG_* e as funções signal(), sigaction(), kill()
 #include <unistd.h>             // Header para as funções alarm(), pause(), sleep(), fork(), exec*() e get*pid()
 #include <sys/wait.h>           // Header para a função wait()
 #include <sys/stat.h>           // Header para as constantes S_ISFIFO e as funções stat() e mkfifo()
 
 #define MAX_ESPERA  5           // Tempo máximo de espera por parte do Cliente
 
 typedef struct {
     char matricula[10];         // Matrícula da viatura
     char pais[3];               // País
     char categoria;             // Categoria da viatura
     char nomeCondutor[80];      // Nome do condutor
 } Viatura;
 
 typedef struct {
     Viatura viatura;            // Viatura a estacionar
     int  pidCliente;            // PID do processo Cliente
     int  pidServidorDedicado;   // PID do processo Servidor Dedicado
 } Estacionamento;
 
 typedef struct {
     Viatura viatura;            // Viatura a estacionar
     char dataEntrada[17];       // Timestamp de Entrada do Cliente no parque, no formato AAAA-MM-DDTHHhmm
     char dataSaida[17];         // Timestamp de Saída do Cliente do parque, no formato AAAA-MM-DDTHHhmm
 } LogItem;
 
 #define FILE_SUFFIX_FIFO ".fifo"                    // Sufixo (extensão) para os nomes dos FIFOs (Named Pipes)
 #define FILE_REQUESTS    "server" FILE_SUFFIX_FIFO  // Nome do FIFO (Named Pipe) que serve para o Cliente fazer os pedidos ao Servidor
 #define FILE_LOGFILE     "estacionamentos.txt"      // Ficheiro para realizar os logs de estacionamento
 #define FILE_USERS       "/etc/passwd"              // Ficheiro onde validar os utilizadores do servidor Tigre
 #define INVALID          -1                         // Utilitário que significa valor inválido
 #define DISPONIVEL       -1                         // Utilitário que significa valor inválido
 
 /* Protótipos de funções */
 void s1_IniciaServidor(int argc, char *argv[]);
 void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque);
 void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento);
 void s1_3_ArmaSinaisServidor();
 void s1_4_CriaFifoServidor(char *filenameFifoServidor);
 void s2_MainServidor();
 void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor);
 void s2_2_LePedidosFifoServidor(FILE *fFifoServidor);
 int  s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest);
 void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD);
 void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD);
 void s3_TrataCtrlC(int sinalRecebido);
 void s4_EncerraServidor(char *filenameFifoServidor);
 void s5_TrataTerminouServidorDedicado(int sinalRecebido);
 void sd7_MainServidorDedicado();
 void sd7_1_ArmaSinaisServidorDedicado();
 void sd7_2_ValidaPidCliente(Estacionamento clientRequest);
 void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD);
 void sd8_1_ValidaMatricula(Estacionamento clientRequest);
 void sd8_2_ValidaPais(Estacionamento clientRequest);
 void sd8_3_ValidaCategoria(Estacionamento clientRequest);
 void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest);
 void sd9_1_AdormeceTempoRandom();
 void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest);
 void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem);
 void sd10_1_AguardaCheckout();
 void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem);
 void sd11_EncerraServidorDedicado();
 void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD);
 void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest);
 void sd12_TrataSigusr2(int sinalRecebido);
 void sd13_TrataSigusr1(int sinalRecebido);
 
 void c1_1_ValidaFifoServidor(char *filenameFifoServidor);
 void c1_2_ArmaSinaisCliente();
 void c2_1_InputEstacionamento(Estacionamento *pclientRequest);
 void c2_2_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor);
 void c2_3_EscrevePedido(FILE *fFifoServidor, Estacionamento clientRequest);
 void c3_ProgramaAlarme(int segundos);
 void c4_1_EsperaRespostaServidor();
 void c4_2_DesligaAlarme();
 void c4_3_InputEsperaCheckout();
 void c5_EncerraCliente();
 void c5_1_EnviaSigusr1AoServidor(Estacionamento clientRequest);
 void c5_2_EsperaRespostaServidorETermina();
 void c6_TrataSigusr1(int sinalRecebido, siginfo_t *siginfo, void *context);
 void c7_TrataSighup(int sinalRecebido);
 void c8_TrataCtrlC(int sinalRecebido);
 void c9_TrataAlarme(int sinalRecebido);
 
 #endif  // __COMMON_H__