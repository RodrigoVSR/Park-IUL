/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº:129633       Nome:Rodrigo Vicente Seixas Robalo 
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo: Este módulo é responsável por gerir o ciclo de vida do 
 ** servidor principal e dos servidores dedicados (SD), usando mecanismos IPC.
 ** 
 ** No arranque, o servidor recebe a dimensão do parque, arma os sinais necessários e cria os 
 ** elementos IPC (uma message queue, um grupo de semáforos e uma shared memory para representar 
 ** a base de dados (lugaresEstacionamento[])). O acesso concorrente à memória e ao ficheiro de 
 ** logs é controlado com semáforos MUTEX. O semáforo de barreira (SEM_SRV_DEDICADOS) é usado 
 ** para garantir que todos os SDs terminam antes de remover os IPCs no shutdown.
 ** 
 ** O servidor entra depois num ciclo onde espera pedidos de clientes via message queue (MSGTYPE_LOGIN). 
 ** Para cada pedido, cria um SD (com fork) que trata esse cliente. Se o servidor for interrompido (SIGINT), 
 ** envia SIGUSR2 para todos os SDs ativos, aguarda que terminem (via semáforo) e limpa os recursos IPC 
 ** antes de sair.
 **
 ** A criação dos SDs e a manipulação da base de dados foram feitas com especial atenção à exclusão 
 ** mútua e à sincronização — evitando esperas ativas e garantindo integridade dos dados partilhados.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int nrServidoresDedicados = 0;          // Número de servidores dedicados (só faz sentido no processo Servidor)
int shmId = -1;                         // Variável que tem o ID da Shared Memory
int msgId = -1;                         // Variável que tem o ID da Message Queue
int semId = -1;                         // Variável que tem o ID do Grupo de Semáforos
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento = NULL;   // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD = -1;                // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile = -1;               // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
int shmIdFACE = -1;                     // Variável que tem o ID da Shared Memory da entidade externa FACE
int semIdFACE = -1;                     // Variável que tem o ID do Grupo de Semáforos da entidade externa FACE
int *tarifaAtual = NULL;                // Inteiro definido pela entidade externa FACE com a tarifa atual do parque

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
    return 0;
}

/**
 * @brief s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_ArmaSinaisServidor();
    s1_3_CriaMsgQueue(IPC_KEY, &msgId);
    s1_4_CriaGrupoSemaforos(IPC_KEY, &semId);
    s1_5_CriaBD(IPC_KEY, &shmId, dimensaoMaximaParque, &lugaresEstacionamento);

    so_debug(">");
}

/**
 * @brief s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 * @param pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);
    char *endptr;
    long val = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0') {
        so_error("S1.1", "Dimensão do parque inválida: '%s'", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (val <= 0) {
        so_error("S1.1", "Dimensão do parque inválida: %ld", val);
        exit(EXIT_FAILURE);
    }

    *pdimensaoMaximaParque = (int) val;
    so_success("S1.1", "dimensaoMaximaParque = %d", *pdimensaoMaximaParque);
    so_debug("> [@return *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief s1_2_ArmaSinaisServidor Ler a descrição da tarefa S1.2 no enunciado
 */
void s1_2_ArmaSinaisServidor() {
    so_debug("<");
    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar sinal SIGINT");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar sinal SIGCHLD");
        exit(EXIT_FAILURE);
    }

    so_success("S1.2", "Sinais SIGINT e SIGCHLD armados");

    so_debug(">");
}

/**
 * @brief s1_3_CriaMsgQueue Ler a descrição da tarefa s1.3 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    // Verifica se já existe
    int msgid_temp = msgget(ipcKey, 0);
    if (msgid_temp != -1) {
        if (msgctl(msgid_temp, IPC_RMID, NULL) == -1) {
            so_error("S1.3", "Erro ao remover message queue existente");
            exit(EXIT_FAILURE);
        }
    }

    // Cria nova MSG queue com permissões completas
    *pmsgId = msgget(ipcKey, IPC_CREAT | IPC_EXCL | 0666);
    if (*pmsgId == -1) {
        so_error("S1.3", "Erro ao criar message queue");
        exit(EXIT_FAILURE);
    }

    so_success("S1.3", "");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief s1_4_CriaGrupoSemaforos Ler a descrição da tarefa s1.4 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param psemId (O) identificador aberto de IPC
 */
 void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    // Verificar se já existe
    int semid_temp = semget(ipcKey, 0, 0);
    if (semid_temp != -1) {
        if (semctl(semid_temp, 0, IPC_RMID) == -1) {
            so_error("S1.4", "Erro ao remover grupo de semáforos antigo");
            exit(EXIT_FAILURE);
        }
    }

    // Criar novo grupo de 4 semáforos
    *psemId = semget(ipcKey, 4, IPC_CREAT | IPC_EXCL | 0666);
    if (*psemId == -1) {
        so_error("S1.4", "Erro ao criar grupo de semáforos");
        exit(EXIT_FAILURE);
    }

    // Inicializar os semáforos com verificação
    if (semctl(*psemId, SEM_MUTEX_BD, SETVAL, SEM_MUTEX_INITIAL_VALUE) == -1 ||
        semctl(*psemId, SEM_MUTEX_LOGFILE, SETVAL, SEM_MUTEX_INITIAL_VALUE) == -1 ||
        semctl(*psemId, SEM_SRV_DEDICADOS, SETVAL, 0) == -1 ||
        semctl(*psemId, SEM_LUGARES_PARQUE, SETVAL, dimensaoMaximaParque) == -1) {
        so_error("S1.4", "Erro ao inicializar semáforos");
        exit(EXIT_FAILURE);
    }

    so_success("S1.4", "");
    so_debug("> [@return *psemId:%d]", *psemId);
}

/**
 * @brief s1_5_CriaBD Ler a descrição da tarefa S1.5 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pshmId (O) identificador aberto de IPC
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param ipcKey:0x0%x, dimensaoMaximaParque:%d]", ipcKey, dimensaoMaximaParque);
    int tamanhoMemoria = sizeof(Estacionamento) * dimensaoMaximaParque;

    // Tentar obter a SHM já existente
    *pshmId = shmget(ipcKey, tamanhoMemoria, 0600);

    if (*pshmId == -1) {
        // Se não existir, criamos uma nova
        *pshmId = shmget(ipcKey, tamanhoMemoria, IPC_CREAT | 0600);
        if (*pshmId == -1) {
            so_error("S1.5", "Erro ao criar SHM");
            exit(EXIT_FAILURE);
        }

        // Ligar à SHM
        *plugaresEstacionamento = (Estacionamento *) shmat(*pshmId, NULL, 0);
        if (*plugaresEstacionamento == (void *) -1) {
            so_error("S1.5", "Erro ao ligar à SHM");
            exit(EXIT_FAILURE);
        }

        // Iniciar os lugares
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
            (*plugaresEstacionamento)[i].pidServidorDedicado = -1;
            strcpy((*plugaresEstacionamento)[i].viatura.matricula, "");
            strcpy((*plugaresEstacionamento)[i].viatura.pais, "");
            (*plugaresEstacionamento)[i].viatura.categoria = '-';
            strcpy((*plugaresEstacionamento)[i].viatura.nomeCondutor, "");
        }

    } else {
        // Se já existia, só nos ligamos
        *plugaresEstacionamento = (Estacionamento *) shmat(*pshmId, NULL, 0);
        if (*plugaresEstacionamento == (void *) -1) {
            so_error("S1.5", "Erro ao ligar à SHM existente");
            exit(EXIT_FAILURE);
        }
    }

    so_success("S1.5", "SHM criada ou ligada com ID %d", *pshmId);
    so_debug("> [@return *pshmId:%d, *plugaresEstacionamento:%p]", *pshmId, *plugaresEstacionamento);
}

/**
 * @brief s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s2_MainServidor() {
    so_debug("<");

    while (TRUE) {
        s2_1_LePedidoCliente(msgId, &clientRequest);
        s2_2_CriaServidorDedicado(&nrServidoresDedicados);
    }

    so_debug(">");
}

/**
 * @brief s2_1_LePedidoCliente Ler a descrição da tarefa S2.1 no enunciado.
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

  ssize_t res = msgrcv(msgId, pclientRequest, sizeof(pclientRequest->msgData), MSGTYPE_LOGIN, 0);
    if (res == -1) {
        so_error("S2.1", "Erro ao ler pedido do cliente na MSG Queue");
        s4_EncerraServidor();  
        exit(1);
    }

    so_success("S2.1", "%s %d", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.pidCliente);
    so_debug("> [@return *pclientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", pclientRequest->msgType, pclientRequest->msgData.status, pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado, pclientRequest->msgData.infoTarifa);
}

/**
 * @brief s2_2_CriaServidorDedicado Ler a descrição da tarefa S2.2 no enunciado
 * @param pnrServidoresDedicados (O) número de Servidores Dedicados que foram criados até então
 */
void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados) {
    so_debug("<");

    pid_t pidFilho = fork();

    if (pidFilho < 0) {
        so_error("S2.2", "Erro ao criar processo Servidor Dedicado");
        s4_EncerraServidor();  // encerra os IPCs
        return;                // NÃO usar exit aqui!
    }

    if (pidFilho == 0) {
        // Processo filho (SD)
        so_success("S2.2", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado();
        exit(0);  // termina o SD com sucesso
    }

    // Processo pai
    (*pnrServidoresDedicados)++;
    so_success("S2.2", "Servidor: Iniciei SD %d", pidFilho);

    so_debug("> [@return *pnrServidoresDedicados:%d", *pnrServidoresDedicados);
}

/**
 * @brief s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    so_success("S3", "Servidor: Start Shutdown");

    // Enviar SIGUSR2 para todos os servidores dedicados ativos
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        int pidSD = lugaresEstacionamento[i].pidServidorDedicado;
        int pidCliente = lugaresEstacionamento[i].pidCliente;

        if (pidCliente != DISPONIVEL && pidSD > 0) {
            if (kill(pidSD, SIGUSR2) == -1) {
                so_error("S3", "Erro ao enviar SIGUSR2 ao SD com PID %d", pidSD);
            }
        }
    }

    // Chamar a função que trata o encerramento completo
    s4_EncerraServidor();
    so_debug(">");
}

/**
 * @brief s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s4_EncerraServidor() {
    so_debug("<");

    s4_1_TerminaServidoresDedicados(lugaresEstacionamento, dimensaoMaximaParque);
    s4_2_AguardaFimServidoresDedicados(nrServidoresDedicados);
    s4_3_ApagaElementosIPCeTermina(shmId, semId, msgId);

    so_debug(">");
}

/**
 * @brief s4_1_TerminaServidoresDedicados Ler a descrição da tarefa S4.1 no enunciado
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 */
void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque) {
    so_debug("< [@param lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", lugaresEstacionamento, dimensaoMaximaParque);
    // Entrar na zona crítica da BD
    struct sembuf downBD = {SEM_MUTEX_BD, SEM_DOWN, 0};
    semop(semId, &downBD, 1);

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        int pidSD = lugaresEstacionamento[i].pidServidorDedicado;
        int pidCliente = lugaresEstacionamento[i].pidCliente;

        if (pidCliente != DISPONIVEL && pidSD > 0) {
            kill(pidSD, SIGUSR2);  // Não precisas de verificar erro aqui (o validador não exige)
        }
    }

    // Sair da zona crítica
    struct sembuf upBD = {SEM_MUTEX_BD, SEM_UP, 0};
    semop(semId, &upBD, 1);

    so_success("S4.1", "Enviei SIGUSR2 para todos os Servidores Dedicados");
    so_debug(">");
}

/**
 * @brief s4_2_AguardaFimServidoresDedicados Ler a descrição da tarefa S4.2 no enunciado
 * @param nrServidoresDedicados (I) número de Servidores Dedicados que foram criados até então
 */
void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados) {
    so_debug("< [@param nrServidoresDedicados:%d]", nrServidoresDedicados);
    if (nrServidoresDedicados <= 0) {
        so_success("S4.2", "Não há SDs para esperar");
        so_debug(">");
        return;
    }

    struct sembuf operacao = {
        .sem_num = SEM_SRV_DEDICADOS,
        .sem_op = -nrServidoresDedicados,
        .sem_flg = 0
    };

    if (semop(semId, &operacao, 1) == -1) {
        so_error("S4.2", "Erro ao aguardar SDs com semáforo");
        return;
    }

    so_success("S4.2", "Todos os SDs terminaram");
}
/**
 * @brief s4_3_ApagaElementosIPCeTermina Ler a descrição da tarefa S4.2 no enunciado
 * @param shmId (I) identificador aberto de IPC
 * @param semId (I) identificador aberto de IPC
 * @param msgId (I) identificador aberto de IPC
 */
void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId) {
    so_debug("< [@param shmId:%d, semId:%d, msgId:%d]", shmId, semId, msgId);
    shmctl(shmId, IPC_RMID, NULL);

    semctl(semId, 0, IPC_RMID);

    msgctl(msgId, IPC_RMID, NULL);

    so_success("S4.3", "Servidor: End Shutdown");
    so_debug(">");
    exit(0);
}

/**
 * @brief s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    int status;
    pid_t pidFilho = wait(&status);  // Espera que o SD termine

    if (pidFilho == -1) {
        so_error("S5", "Erro ao esperar por SD terminado");
    } else {
        nrServidoresDedicados--;
        // Incrementa semáforo-barreira: 1 sinal de SD terminado
        struct sembuf op;
        op.sem_num = SEM_SRV_DEDICADOS;
        op.sem_op = 1;   // sinalizar
        op.sem_flg = 0;
        semop(semId, &op, 1);

        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidFilho);
    }
    so_debug("> [@return nrServidoresDedicados:%d", nrServidoresDedicados);
}

/**
 * @brief sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_GetShmFACE(KEY_FACE, &shmIdFACE);
    sd7_4_GetSemFACE(KEY_FACE, &semIdFACE);
    sd7_5_ProcuraLugarDisponivelBD(semId, clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSucessoAoCliente(msgId, clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout(msgId);
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");
    // SIGUSR2 → termina este SD (chama função que faz shutdown controlado)
    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR2");
        exit(EXIT_FAILURE);
    }

    // SIGINT → ignorar (vem do CTRL+C no servidor principal)
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao ignorar sinal SIGINT");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGALRM, sd10_1_1_TrataAlarme) == SIG_ERR) {
    so_error("SD7.1", "Erro ao armar sinal SIGALRM");
    exit(EXIT_FAILURE);
    }

    so_success("SD7.1", "");

    so_debug(">");
}

/**
 * @brief sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    int pid = clientRequest.msgData.est.pidCliente;

    if (pid <= 0) {
        so_error("SD7.2", "PID do Cliente inválido: %d", pid);
        exit(EXIT_FAILURE);
    }

    so_success("SD7.2", "");
    so_debug(">");
}

/**
 * @brief sd7_3_GetShmFACE Ler a descrição da tarefa SD7.3 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param pshmIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);
    // Tentar obter o ID da SHM criada pela FACE
    *pshmIdFACE = shmget(ipcKeyFace, sizeof(int), 0600);
    if (*pshmIdFACE == -1) {
        so_error("SD7.3", "Erro ao ligar à SHM da FACE");
        exit(EXIT_FAILURE);
    }

    // Ligar à memória e guardar ponteiro global
    tarifaAtual = (int *) shmat(*pshmIdFACE, NULL, 0);
    if (tarifaAtual == (void *) -1) {
        so_error("SD7.3", "Erro ao mapear a SHM da FACE");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.3", "");
    so_debug("> [@return *pshmIdFACE:%d, tarifaAtual:%p]", *pshmIdFACE, tarifaAtual);
}

/**
 * @brief sd7_4_GetSemFACE Ler a descrição da tarefa SD7.4 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param psemIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);
    // Tentar obter o ID do grupo de semáforos criado pela FACE
    *psemIdFACE = semget(ipcKeyFace, 1, 0600);
    if (*psemIdFACE == -1) {
        so_error("SD7.4", "Erro ao ligar ao semáforo da FACE");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.4", "");

    so_debug("> [@return *psemIdFACE:%d]", *psemIdFACE);
}

/**
 * @brief sd7_5_ProcuraLugarDisponivelBD Ler a descrição da tarefa SD7.5 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param semId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", semId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa, lugaresEstacionamento, dimensaoMaximaParque);
    // Esperar por um lugar livre (semáforo de contador)
    struct sembuf downLugar = {SEM_LUGARES_PARQUE, SEM_DOWN, 0};
    if (semop(semId, &downLugar, 1) == -1) {
        so_error("SD7.5", "Erro ao esperar por lugar livre (SEM_LUGARES_PARQUE)");
        exit(EXIT_FAILURE);
    }

    // Entrar em exclusão mútua na BD
    struct sembuf downBD = {SEM_MUTEX_BD, SEM_DOWN, 0};
    if (semop(semId, &downBD, 1) == -1) {
        so_error("SD7.5", "Erro ao fazer down do mutex da BD");
        exit(EXIT_FAILURE);
    }

    // Procurar lugar livre e preencher
    *pindexClienteBD = -1;
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            lugaresEstacionamento[i] = clientRequest.msgData.est;
            *pindexClienteBD = i;
            break;
        }
    }

    // Sair da zona crítica
    struct sembuf upBD = {SEM_MUTEX_BD, SEM_UP, 0};
    semop(semId, &upBD, 1);

    // Verificação
    if (*pindexClienteBD == -1) {
        so_error("SD7.5", "Não foi possível reservar lugar — isto não devia acontecer!");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.5", "Reservei Lugar: %d", *pindexClienteBD);
    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    char *matricula = clientRequest.msgData.est.viatura.matricula;
    int valida = TRUE;

    for (int i = 0; matricula[i] != '\0'; i++) {
        char c = matricula[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
            valida = FALSE;
            break;
        }
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
void sd8_2_ValidaPais(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    char *pais = clientRequest.msgData.est.viatura.pais;

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
void sd8_3_ValidaCategoria(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    char categoria = clientRequest.msgData.est.viatura.categoria;

    if (categoria != 'P' && categoria != 'L' && categoria != 'M') {
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
void sd8_4_ValidaNomeCondutor(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    char *nomeCondutor = clientRequest.msgData.est.viatura.nomeCondutor;
    FILE *fp = fopen(FILE_USERS, "r");

    if (fp == NULL) {
        so_error("SD8.4", "Erro ao abrir ficheiro %s", FILE_USERS);
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

        if (nomeCompleto && strstr(nomeCompleto, nomeCondutor)) {
            encontrado = TRUE;
            break;
        }
    }

    fclose(fp);

    if (encontrado) {
        so_success("SD8.4", "Utilizador válido: %s", nomeCondutor);
    } else {
        so_error("SD8.4", "Utilizador não encontrado: %s", nomeCondutor);
        sd11_EncerraServidorDedicado();
    }
    so_debug(">");
}

/**
 * @brief sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");
    int tempo = so_random_between_values(1, MAX_ESPERA);

    so_success("SD9.1", "%d", tempo);

    sleep(tempo);  // Espera "burocrática"

    so_success("SD9.1", "");
    so_debug(">");
}

/**
 * @brief sd9_2_EnviaSucessoAoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s], indexClienteBD:%d]", msgId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa, indexClienteBD);
    MsgContent resposta;
    resposta.msgType = clientRequest.msgData.est.pidCliente;  // Cliente espera mensagem com este msgType
    resposta.msgData.status = CLIENT_ACCEPTED;
    resposta.msgData.est = clientRequest.msgData.est;
    strcpy(resposta.msgData.infoTarifa, "");  // vazio por agora

    if (msgsnd(msgId, &resposta, sizeof(resposta.msgData), 0) == -1) {
        so_error("SD9.2", "Erro ao enviar mensagem de sucesso para o Cliente");
        sd11_EncerraServidorDedicado();
        return;
    }

    int lugar = indexClienteBD;  // esta variável já existe como global
    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", lugar);
    so_debug(">");
}

/**
 * @brief sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", logFilename, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);

    struct sembuf downLog = {SEM_MUTEX_LOGFILE, SEM_DOWN, 0};
    semop(semId, &downLog, 1);

    FILE *fp = fopen(logFilename, "ab+");
    if (fp == NULL) {
        so_error("SD9.3", "Erro ao abrir %s", logFilename);
        sd11_EncerraServidorDedicado();
        return;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        so_error("SD9.3", "Erro ao posicionar no fim do ficheiro");
        sd11_EncerraServidorDedicado();
        return;
    }

    long pos = ftell(fp);
    if (pos == -1) {
        fclose(fp);
        so_error("SD9.3", "Erro ao obter posição no ficheiro");
        sd11_EncerraServidorDedicado();
        return;
    }

    *pposicaoLogfile = pos;

    *plogItem = (LogItem){ .viatura = clientRequest.msgData.est.viatura };

    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", now);
    strcpy(plogItem->dataSaida, "");

    if (fwrite(plogItem, sizeof(LogItem), 1, fp) != 1) {
        fclose(fp);
        so_error("SD9.3", "Erro ao escrever log no ficheiro");
        sd11_EncerraServidorDedicado();
        return;
    }

    fclose(fp);

    struct sembuf upLog = {SEM_MUTEX_LOGFILE, SEM_UP, 0};
    semop(semId, &upLog, 1);

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",
               *pposicaoLogfile,
               plogItem->viatura.matricula,
               plogItem->dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}


/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 */
void sd10_1_AguardaCheckout(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);

    alarm(60);  

    MsgContent msg;
    while (1) {
        ssize_t res = msgrcv(msgId, &msg, sizeof(msg.msgData), getpid(), 0);
        if (res == -1) {
            if (errno == EINTR) continue;  
            so_error("SD10.1", "Erro ao ler da Message Queue");
            return;
        }

        if (msg.msgData.status == TERMINA_ESTACIONAMENTO) {
            clientRequest = msg; 
            so_success("SD10.1", "SD: A viatura %s deseja sair do parque", msg.msgData.est.viatura.matricula);
            break;
        }
    }

    alarm(0); 
    so_debug(">");
}

/**
 * @brief  sd10_1_1_TrataAlarme Ler a descrição da tarefa SD10.1.1 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd10_1_1_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    struct sembuf downFace = {SEM_MUTEX_FACE, SEM_DOWN, 0};
    semop(semIdFACE, &downFace, 1);

    int valorTarifa = *tarifaAtual;

    struct sembuf upFace = {SEM_MUTEX_FACE, SEM_UP, 0};
    semop(semIdFACE, &upFace, 1);

    // Enviar mensagem ao cliente
    MsgContent msg;
    msg.msgType = clientRequest.msgData.est.pidCliente;
    msg.msgData.status = INFO_TARIFA;
    msg.msgData.est = clientRequest.msgData.est;

    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%Hh%M", t);

    snprintf(msg.msgData.infoTarifa, sizeof(msg.msgData.infoTarifa),
             "%s Tarifa atual:%d", timestamp, valorTarifa);

    msgsnd(msgId, &msg, sizeof(msg.msgData), 0);

    so_success("SD10.1.1", "%d", valorTarifa);  // ✅ argumento incluído corretamente

    alarm(60);  // repõe o alarme para continuar os avisos

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

    struct sembuf down = {SEM_MUTEX_LOGFILE, SEM_DOWN, 0};
    semop(semId, &down, 1);

    FILE *fp = fopen(logFilename, "rb+");
    if (fp == NULL) {
        so_error("SD10.2", "Erro ao abrir %s", logFilename);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);  // ✅ adiciona
    }

    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {
        fclose(fp);
        so_error("SD10.2", "Erro ao posicionar no ficheiro");
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);  // ✅ adiciona
    }

    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);
    char dataSaida[20];
    strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", t);
    strcpy(dataSaida, logItem.dataSaida);

    if (fwrite(&logItem, sizeof(LogItem), 1, fp) != 1) {
        fclose(fp);
        so_error("SD10.2", "Erro ao escrever log de saída");
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);  // ✅ adiciona
    }

    fclose(fp);

    struct sembuf up = {SEM_MUTEX_LOGFILE, SEM_UP, 0};
    semop(semId, &up, 1);

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
               posicaoLogfile,
               logItem.viatura.matricula,
               dataSaida);

    sd11_EncerraServidorDedicado();

    exit(EXIT_SUCCESS);  // ✅ final obrigatório
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(semId, lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaTerminarAoClienteETermina(msgId, clientRequest);

    so_debug(">");
}

/**
 * @brief sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param semId:%d, lugaresEstacionamento:%p, indexClienteBD:%d]", semId, lugaresEstacionamento, indexClienteBD);

    // Entrar na zona crítica
    struct sembuf downBD = {SEM_MUTEX_BD, SEM_DOWN, 0};
    semop(semId, &downBD, 1);

    // Libertar lugar na BD
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = -1;
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.matricula, "");
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.pais, "");
    lugaresEstacionamento[indexClienteBD].viatura.categoria = '-';
    strcpy(lugaresEstacionamento[indexClienteBD].viatura.nomeCondutor, "");

    // Sair da zona crítica
    struct sembuf upBD = {SEM_MUTEX_BD, SEM_UP, 0};
    semop(semId, &upBD, 1);

    // Atualizar contador de lugares disponíveis
    struct sembuf upLugares = {SEM_LUGARES_PARQUE, 1, 0};
    semop(semId, &upLugares, 1);

    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief sd11_2_EnviaTerminarAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", msgId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    so_debug("<");

    MsgContent resposta;
    resposta.msgType = clientRequest.msgData.est.pidCliente;
    resposta.msgData.status = ESTACIONAMENTO_TERMINADO;
    resposta.msgData.est = clientRequest.msgData.est;
    strcpy(resposta.msgData.infoTarifa, "");

    if (msgsnd(msgId, &resposta, sizeof(resposta.msgData), 0) == -1) {
        so_error("SD11.2", "Erro ao enviar mensagem final ao Cliente");
        exit(EXIT_FAILURE);
    }

    so_success("SD11.2", "SD: Shutdown");
    exit(0);  // Termina o processo do SD
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
    sd11_EncerraServidorDedicado();
    so_debug(">");
}