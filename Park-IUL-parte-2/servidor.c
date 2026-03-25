/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
 **
 ** Aluno: Nº:129633       Nome: Rodrigo Vicente Seixas Robalo 
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 ** Este módulo é responsável por gerir o parque de estacionamento, recebendo pedidos 
 ** dos clientes através de um FIFO. Cria processos servidores dedicados para cada viatura,
 ** acompanha a ocupação dos lugares, e lida com sinais para gerir entradas, saídas e encerramento do parque.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento;  // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD;                     // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile;                    // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile

/**
 * @brief  Processamento do processo Servidor e dos processos Servidor Dedicado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @return Success (0) or not (<> 0)
 */
int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv); 
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief  s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_CriaBD(dimensaoMaximaParque, &lugaresEstacionamento);
    s1_3_ArmaSinaisServidor();
    s1_4_CriaFifoServidor(FILE_REQUESTS);

    so_debug(">");
}

/**
 * @brief  s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @param  pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
 void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if (argc != 2) {
        so_error("S1.1", "Número de argumentos inválido. Uso: ./servidor <dimensao>");
        exit(EXIT_FAILURE);
    }

    *pdimensaoMaximaParque = atoi(argv[1]);

    if (*pdimensaoMaximaParque <= 0) {
        so_error("S1.1", "Dimensão inválida do parque: %d", *pdimensaoMaximaParque);
        exit(EXIT_FAILURE);
    }

    so_success("S1.1", "Dimensão do parque: %d", *pdimensaoMaximaParque);
    so_debug("> [@param +pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief  s1_2_CriaBD Ler a descrição da tarefa S1.2 no enunciado
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
 void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param dimensaoMaximaParque:%d]", dimensaoMaximaParque);

    *plugaresEstacionamento = (Estacionamento *) malloc(sizeof(Estacionamento) * dimensaoMaximaParque);
    if (*plugaresEstacionamento == NULL) {
        so_error("S1.2", "Erro ao alocar memória para o array de estacionamento");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
        (*plugaresEstacionamento)[i].pidServidorDedicado = -1;
        strcpy((*plugaresEstacionamento)[i].viatura.matricula, "");
        strcpy((*plugaresEstacionamento)[i].viatura.pais, "");
        (*plugaresEstacionamento)[i].viatura.categoria = '-';
        strcpy((*plugaresEstacionamento)[i].viatura.nomeCondutor, "");
    }

    so_success("S1.2", "Base de dados criada com %d lugares", dimensaoMaximaParque);
    so_debug("> [*plugaresEstacionamento:%p]", *plugaresEstacionamento);
}


/**
 * @brief  s1_3_ArmaSinaisServidor Ler a descrição da tarefa S1.3 no enunciado
 */
void s1_3_ArmaSinaisServidor() {
    so_debug("<");

    // Arma SIGINT para shutdown controlado
    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.3", "Erro ao capturar sinal SIGINT.");
        exit(EXIT_FAILURE);
    }

    // Arma SIGCHLD para tratar fim de processos filho (Servidores Dedicados)
    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.3", "Erro ao capturar sinal SIGCHLD.");
        exit(EXIT_FAILURE);
    }

    so_success("S1.3", "Sinais SIGINT e SIGCHLD ligados com sucesso");

    so_debug(">");
}


/**
 * @brief  s1_4_CriaFifoServidor Ler a descrição da tarefa S1.4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
 void s1_4_CriaFifoServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    // Se o FIFO já existe, remove-o
    if (access(filenameFifoServidor, F_OK) == 0) {
        if (unlink(filenameFifoServidor) == -1) {
            so_error("S1.4", "Erro ao remover FIFO existente: %s", filenameFifoServidor);
            exit(EXIT_FAILURE);
        }
    }

    // Criar o FIFO com permissões 0666
    if (mkfifo(filenameFifoServidor, 0666) == -1) {
        so_error("S1.4", "Erro ao criar FIFO: %s", filenameFifoServidor);
        exit(EXIT_FAILURE);
    }

    so_success("S1.4", "FIFO criado: %s", filenameFifoServidor);
    so_debug(">");
}


/**
 * @brief  s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO, exceto depois de
 *         realizada a função s2_1_AbreFifoServidor(), altura em que podem
 *         comentar o statement sleep abaixo (que, neste momento está aqui
 *         para evitar que os alunos tenham uma espera ativa no seu código)
 */
 void s2_MainServidor() {
    so_debug("<");

    FILE *fFifoServidor;
    while (TRUE) { 
        s2_1_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
        s2_2_LePedidosFifoServidor(fFifoServidor);
        sleep(1);
    }

    so_debug(">");
}

/**
 * @brief  s2_1_AbreFifoServidor Ler a descrição da tarefa S2.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    int fd = open(filenameFifoServidor, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        so_error("S2.1", "Erro ao abrir FIFO do servidor para leitura");
        *pfFifoServidor = NULL;
        s4_EncerraServidor(filenameFifoServidor);
        return;
    }

    *pfFifoServidor = fdopen(fd, "rb");
    if (*pfFifoServidor == NULL) {
        close(fd);
        so_error("S2.1", "Erro ao associar FILE* ao FIFO");
        s4_EncerraServidor(filenameFifoServidor);
        return;
    }

    so_success("S2.1", "");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}


/**
 * @brief  s2_2_LePedidosFifoServidor Ler a descrição da tarefa S2.2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_2_LePedidosFifoServidor(FILE *fFifoServidor) {
    so_debug("<");

    int terminaCiclo2 = FALSE;
    while (TRUE) {
        terminaCiclo2 = s2_2_1_LePedido(fFifoServidor, &clientRequest);
        if (terminaCiclo2)
            break;
        s2_2_2_ProcuraLugarDisponivelBD(clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
        s2_2_3_CriaServidorDedicado(lugaresEstacionamento, indexClienteBD);
    }

    so_debug(">");
}

/**
 * @brief  s2_2_1_LePedido Ler a descrição da tarefa S2.2.1 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  pclientRequest (O) pedido recebido, enviado por um Cliente
 * @return TRUE se não conseguiu ler um pedido porque o FIFO não tem mais pedidos.
 */
int s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest) {
    int naoHaMaisPedidos = TRUE;
    so_debug("< [@param fFifoServidor:%p]", fFifoServidor);

    size_t bytesLidos = fread(pclientRequest, sizeof(Estacionamento), 1, fFifoServidor);

    if (bytesLidos == 1) {
        naoHaMaisPedidos = FALSE;
        so_success("S2.2.1", "Li Pedido do FIFO");
    } else {
        if (feof(fFifoServidor)) {
            so_success("S2.2.1", "Não há mais registos no FIFO");
        } else {
            so_error("S2.2.1", "Erro ao ler do FIFO");
            s4_EncerraServidor(FILE_REQUESTS);
            exit(EXIT_FAILURE);
        }
    }
    so_debug("> [naoHaMaisPedidos:%d, *pclientRequest:[%s:%s:%c:%s:%d.%d]]", naoHaMaisPedidos, pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
    return naoHaMaisPedidos;
}

/**
 * @brief  s2_2_2_ProcuraLugarDisponivelBD Ler a descrição da tarefa S2.2.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);
    *pindexClienteBD = -1;

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            *pindexClienteBD = i;
            lugaresEstacionamento[i] = clientRequest;
            break;
        }
    }

    if (*pindexClienteBD != -1) {
        so_success("S2.2.2", "Reservei Lugar: %d", *pindexClienteBD);
    } else {
        so_error("S2.2.2", "Parque cheio. Nenhum lugar disponível.");
    }
    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  s2_2_3_CriaServidorDedicado    Ler a descrição da tarefa S2.2.3 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);
    pid_t pidFilho = fork();

    if (pidFilho < 0) {
        so_error("S2.2.3", "Erro ao criar processo Servidor Dedicado");
        s4_EncerraServidor(FILE_REQUESTS);
        exit(EXIT_FAILURE);
    }
    if (pidFilho == 0) {
        so_success("S2.2.3", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado();
        exit(0);
    }
    if (indexClienteBD >= 0) {
        lugaresEstacionamento[indexClienteBD].pidServidorDedicado = pidFilho;
        so_success("S2.2.3", "Servidor: Iniciei SD %d", pidFilho);
    }
    so_debug(">");
}

/**
 * @brief  s3_TrataCtrlC    Ler a descrição da tarefa S3 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente != DISPONIVEL) {
            pid_t pidSD = lugaresEstacionamento[i].pidServidorDedicado;

            if (pidSD > 0) {
                if (kill(pidSD, SIGUSR2) == -1) {
                    so_error("S3", "Erro ao enviar SIGUSR2 para SD %d", pidSD);
                    s4_EncerraServidor(FILE_REQUESTS);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    s4_EncerraServidor(FILE_REQUESTS);
}

/**
 * @brief  s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void s4_EncerraServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);
    if (unlink(filenameFifoServidor) == -1) {
        so_error("S4", "Erro ao remover FIFO %s", filenameFifoServidor);
        exit(0);
    }

    so_success("S4", "Servidor: End Shutdown");
    so_debug(">");
    exit(0);
}

/**
 * @brief  s5_TrataTerminouServidorDedicado    Ler a descrição da tarefa S5 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    int status;
    pid_t pidFilhoTerminou = wait(&status);  // ← bloqueia até um SD terminar

    if (pidFilhoTerminou == -1) {
        so_error("S5", "Erro ao esperar pelo Servidor Dedicado");
    } else {
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidFilhoTerminou);
    }
    so_debug(">");
}

/**
 * @brief  sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_ValidaLugarDisponivelBD(indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSigusr1AoCliente(clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout();
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");

    so_debug(">");
}

/**
 * @brief  sd7_1_ArmaSinaisServidorDedicado    Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");
    struct sigaction sa;

    sa.sa_handler = sd12_TrataSigusr2; // Armar SIGUSR2 → shutdown vindo do Servidor
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR2");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sd13_TrataSigusr1; // Armar SIGUSR1 → pedido de saída vindo do Cliente
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR1");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.1", "");
    so_debug(">");
}

/**
 * @brief  sd7_2_ValidaPidCliente    Ler a descrição da tarefa SD7.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    if (clientRequest.pidCliente <= 0) {
        so_error("SD7.2", "PID do Cliente inválido: %d", clientRequest.pidCliente);
        exit(EXIT_FAILURE);
    }
    so_success("SD7.2", "");
    so_debug(">");
}

/**
 * @brief  sd7_3_ValidaLugarDisponivelBD    Ler a descrição da tarefa SD7.3 no enunciado
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD) {
    so_debug("< [@param indexClienteBD:%d]", indexClienteBD);
    if (indexClienteBD < 0) {
        pid_t pidCliente = clientRequest.pidCliente;

        if (pidCliente > 0) {
            int resultadoEnvio = kill(pidCliente, SIGHUP);

            if (resultadoEnvio == -1) {
                so_error("SD7.3", "Erro ao enviar SIGHUP ao cliente com PID %d", pidCliente);
                exit(EXIT_FAILURE);
            }
        }

        so_success("SD7.3", "SD: Parque cheio. Pedido rejeitado.");
        exit(EXIT_SUCCESS);
    }

    so_success("SD7.3", "");
    so_debug(">");
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    char *matricula = clientRequest.viatura.matricula;
    int i = 0;
    int valida = 1;

    while (matricula[i] != '\0') {
        if (!(matricula[i] >= 'A' && matricula[i] <= 'Z') &&
            !(matricula[i] >= '0' && matricula[i] <= '9')) {
            valida = 0;
            break;
        }
        i++;
    }

    if (!valida) {
        so_error("SD8.1", "Matrícula inválida: %s", matricula);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.1", "");
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    char *pais = clientRequest.viatura.pais;

    if (strlen(pais) != 2 || 
        !(pais[0] >= 'A' && pais[0] <= 'Z') || 
        !(pais[1] >= 'A' && pais[1] <= 'Z')) {

        so_error("SD8.2", "Código de país inválido: %s", pais);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.2", "");
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    char categoria = clientRequest.viatura.categoria;

    if (categoria != 'L' && categoria != 'P' && categoria != 'M') {
        so_error("SD8.3", "Categoria inválida: %c", categoria);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.3", "");
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    FILE *fp = fopen("_etc_passwd", "r");
    if (fp == NULL) {
        so_error("SD8.4", "Erro ao abrir ficheiro _etc_passwd");
        sd11_EncerraServidorDedicado();
        return;
    }

    char linha[512];
    int encontrado = FALSE;

    while (fgets(linha, sizeof(linha), fp) != NULL) {
        char *token = strtok(linha, ":");
        int campo = 0;
        char *nomeCompleto = NULL;

        while (token != NULL) {
            if (campo == 4) {
                nomeCompleto = token;
                break;
            }
            token = strtok(NULL, ":");
            campo++;
        }

        if (nomeCompleto != NULL && strstr(nomeCompleto, clientRequest.viatura.nomeCondutor)) {
            encontrado = TRUE;
            break;
        }
    }

    fclose(fp);

    if (encontrado) {
        so_success("SD8.4", "Utilizador válido: %s", clientRequest.viatura.nomeCondutor);
    } else {
        so_error("SD8.4", "Utilizador não encontrado: %s", clientRequest.viatura.nomeCondutor);
        sd11_EncerraServidorDedicado();
    }
    so_debug(">");
}

/**
 * @brief  sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");
    int tempo = so_random_between_values(1, MAX_ESPERA);

    so_success("SD9.1", "(SD9.1) %d", tempo);

    sleep(tempo);

    so_success("SD9.1", "");
    so_debug(">");
}

/**
 * @brief  sd9_2_EnviaSigusr1AoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    if (kill(clientRequest.pidCliente, SIGUSR1) == -1) {
        so_error("SD9.2", "Erro ao enviar sinal SIGUSR1 para o cliente %d", clientRequest.pidCliente);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);

    so_debug(">");
}

/**
 * @brief  sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    FILE *fp = fopen(logFilename, "ab");
    if (fp == NULL) {
        so_error("SD9.3", "Erro ao abrir ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    *pposicaoLogfile = ftell(fp);

    int i;

    for (i = 0; i < 10; i++) {
        plogItem->viatura.matricula[i] = clientRequest.viatura.matricula[i];
    }

    for (i = 0; i < 3; i++) {
        plogItem->viatura.pais[i] = clientRequest.viatura.pais[i];
    }

    plogItem->viatura.categoria = clientRequest.viatura.categoria;

    for (i = 0; i < 80; i++) {
        plogItem->viatura.nomeCondutor[i] = clientRequest.viatura.nomeCondutor[i];
    }

    time_t agora = time(NULL);
    struct tm *infoTempo = localtime(&agora);

    snprintf(plogItem->dataEntrada, sizeof(plogItem->dataEntrada),
             "%04d-%02d-%02dT%02dh%02d",
             1900 + infoTempo->tm_year,
             1 + infoTempo->tm_mon,
             infoTempo->tm_mday,
             infoTempo->tm_hour,
             infoTempo->tm_min);

    for (i = 0; i < sizeof(plogItem->dataSaida); i++) {
        plogItem->dataSaida[i] = '\0';
    }

    size_t resultado = fwrite(plogItem, sizeof(LogItem), 1, fp);
    fclose(fp);

    if (resultado != 1) {
        so_error("SD9.3", "Erro ao escrever no ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",
        *pposicaoLogfile,
        plogItem->viatura.matricula,
        plogItem->dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 */
void sd10_1_AguardaCheckout() {
    so_debug("<");
    pause();
    so_success("SD10.1", "SD: A viatura %s deseja sair do parque", clientRequest.viatura.matricula);
    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]", logFilename, posicaoLogfile, logItem.viatura.matricula, logItem.viatura.pais, logItem.viatura.categoria, logItem.viatura.nomeCondutor, logItem.dataEntrada, logItem.dataSaida);
    FILE *fp = fopen(logFilename, "rb+"); // abre o ficheiro em modo leitura+escrita binário
    if (fp == NULL) {
        so_error("SD10.2", "Erro ao abrir ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) { // posiciona-se na posição do registo de entrada
        fclose(fp);
        so_error("SD10.2", "Erro ao posicionar no ficheiro");
        sd11_EncerraServidorDedicado();
        return;
    }

    time_t agora = time(NULL); // atualiza o campo dataSaida com timestamp atual
    struct tm *infoTempo = localtime(&agora);

    snprintf(logItem.dataSaida, sizeof(logItem.dataSaida),
             "%04d-%02d-%02dT%02dh%02d",
             1900 + infoTempo->tm_year,
             1 + infoTempo->tm_mon,
             infoTempo->tm_mday,
             infoTempo->tm_hour,
             infoTempo->tm_min);

    size_t resultado = fwrite(&logItem, sizeof(LogItem), 1, fp); // reescreve o LogItem atualizado

    fclose(fp);

    if (resultado != 1) {
        so_error("SD10.2", "Erro ao escrever log de saída no ficheiro %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
        posicaoLogfile,
        logItem.viatura.matricula,
        logItem.dataSaida);
        so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaSighupAoClienteETermina(clientRequest);

    so_debug(">");
}

/**
 * @brief  sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);
    if (indexClienteBD < 0) {
        so_error("SD11.1", "Índice de lugar inválido: %d", indexClienteBD);
        return;
    }
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = -1;
    int i;
    for (i = 0; i < 10; i++) {
        lugaresEstacionamento[indexClienteBD].viatura.matricula[i] = '\0';
    }
    for (i = 0; i < 3; i++) {
        lugaresEstacionamento[indexClienteBD].viatura.pais[i] = '\0';
    }
    lugaresEstacionamento[indexClienteBD].viatura.categoria = '-';
    for (i = 0; i < 80; i++) {
        lugaresEstacionamento[indexClienteBD].viatura.nomeCondutor[i] = '\0';
    }
    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief  sd11_2_EnviaSighupAoClienteETerminaSD Ler a descrição da tarefa SD11.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    if (kill(clientRequest.pidCliente, SIGHUP) == -1) {
        so_error("SD11.2", "Erro ao enviar SIGHUP ao cliente %d", clientRequest.pidCliente);
    } else {
        so_success("SD11.2", "SD: Shutdown");
    }

    so_debug(">");
    exit(0); 
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
    sd11_EncerraServidorDedicado(); //limpa e termina o SD
    so_debug(">");
}

/**
 * @brief  sd13_TrataSigusr1    Ler a descrição da tarefa SD13 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd13_TrataSigusr1(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    so_success("SD13", "SD: Recebi pedido do Cliente para terminar o estacionamento");
    so_debug(">");
}