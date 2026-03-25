/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº:129633       Nome:Rodrgo Vicente Seixas Robalo 
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo: Este módulo representa a interação entre o utilizador 
 ** e o sistema de estacionamento. Ao iniciar, liga-se à message queue (já criada pelo servidor) 
 ** e arma os sinais SIGINT e SIGALRM.
 **
 ** Depois, pede ao utilizador os dados da viatura, valida-os e envia o pedido ao servidor. É configurado 
 ** um alarme com MAX_ESPERA segundos para limitar o tempo de espera pela resposta inicial. Se o 
 ** cliente não receber nada nesse tempo, o SIGALRM é ativado e o processo termina com erro.
 **
 ** Caso o servidor aceite o pedido (CLIENT_ACCEPTED), o cliente entra num ciclo onde escuta mensagens 
 ** enviadas pelo SD com o status INFO_TARIFA (informações periódicas sobre o valor da tarifa) ou 
 ** ESTACIONAMENTO_TERMINADO (fim do estacionamento). O utilizador pode ainda cancelar manualmente com 
 ** ctrl c, o que envia uma mensagem ao SD para terminar o estacionamento.
 **
 ** O módulo foi implementado de forma a não depender de esperas ativas e a responder corretamente 
 ** a sinais e mensagens, garantindo sincronização eficaz com o servidor e reatividade à ação do utilizador.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int msgId = -1;                         // Variável que tem o ID da Message Queue
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief Processamento do processo Cliente.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_GetMsgQueue(IPC_KEY, &msgId);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    c2_2_EscrevePedido(msgId, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor(msgId, &clientRequest);
    c4_2_DesligaAlarme();

    c5_MainCliente(msgId, &clientRequest);

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief c1_1_GetMsgQueue Ler a descrição da tarefa C1.1 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);
    *pmsgId = msgget(ipcKey, 0600);
    if (*pmsgId == -1) {
        so_error("C1.1", "Erro ao ligar à Message Queue");
        exit(EXIT_FAILURE);
    }

    so_success("C1.1", "Ligado à MQ com ID %d", *pmsgId);
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.2 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");
    // Armamos o SIGINT (CTRL+C)
    if (signal(SIGINT, c6_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar SIGINT");
        exit(EXIT_FAILURE);
    }

    // Armamos o SIGALRM (timeout)
    if (signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar SIGALRM");
        exit(EXIT_FAILURE);
    }

    so_success("C1.2", "Sinais armados no Cliente");
    so_debug(">");
}

/**
 * @brief c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(MsgContent *pclientRequest) {
    so_debug("<");

    // Limpa toda a estrutura
    memset(pclientRequest, 0, sizeof(MsgContent));

    Estacionamento *est = &pclientRequest->msgData.est;

    printf("Introduza a matrícula da viatura: ");
    scanf("%9s", est->viatura.matricula);

    printf("Introduza o país da viatura (2 letras): ");
    scanf("%2s", est->viatura.pais);

    printf("Introduza a categoria da viatura (P/L/M): ");
    scanf(" %c", &est->viatura.categoria);

    getchar(); // limpar '\n' que ficou no buffer

    printf("Introduza o nome do condutor: ");
    fgets(est->viatura.nomeCondutor, sizeof(est->viatura.nomeCondutor), stdin);
    est->viatura.nomeCondutor[strcspn(est->viatura.nomeCondutor, "\n")] = '\0';

    est->pidCliente = getpid();
    est->pidServidorDedicado = -1;

    pclientRequest->msgType = MSGTYPE_LOGIN;
    pclientRequest->msgData.status = -1;
    strcpy(pclientRequest->msgData.infoTarifa, "");

    // Validação mínima
    if (strlen(est->viatura.matricula) == 0 ||
        strlen(est->viatura.pais) != 2 ||
        (est->viatura.categoria != 'P' && est->viatura.categoria != 'L' && est->viatura.categoria != 'M') ||
        strlen(est->viatura.nomeCondutor) == 0) {
        so_error("C2.1", "Dados inválidos introduzidos");
        exit(1);
    }

    // Mensagem de sucesso como o validador espera
    so_success("C2.1", "%s %s %c %s %d %d",
               est->viatura.matricula,
               est->viatura.pais,
               est->viatura.categoria,
               est->viatura.nomeCondutor,
               est->pidCliente,
               est->pidServidorDedicado);

    so_debug("> [*pclientRequest:[...]]");
}

/**
 * @brief c2_2_EscrevePedido Ler a descrição da tarefa C2.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_2_EscrevePedido(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", msgId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), 0) == -1) {
        so_error("C2.2", "Erro ao enviar pedido para o servidor");
        exit(EXIT_FAILURE);
    }

    so_success("C2.2", "");
    so_debug(">");
}

/**
 * @brief c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);
        alarm(segundos);  // Inicia alarme
    so_success("C3", "Espera resposta em %d segundos", segundos);
    so_debug(">");
}

/**
 * @brief c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);
    while (TRUE) {
        if (msgrcv(msgId, pclientRequest, sizeof(pclientRequest->msgData), getpid(), 0) == -1) {
            so_error("C4.1", "Erro ao ler resposta da MSG");
            exit(EXIT_FAILURE);
        }

        int status = pclientRequest->msgData.status;

        if (status == CLIENT_ACCEPTED) {
            so_success("C4.1", "Check-in realizado com sucesso");
            break;
        } else if (status == ESTACIONAMENTO_TERMINADO) {
            so_success("C4.1", "Não é possível estacionar");
            exit(0);
        }

        // Caso seja outro status, ignora (ex: INFO_TARIFA antes do tempo)
    }

    so_debug("> [*pclientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", pclientRequest->msgType, pclientRequest->msgData.status, pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado, pclientRequest->msgData.infoTarifa);
}

/**
 * @brief c4_2_DesligaAlarme Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");
    alarm(0);  // Cancela alarme
    so_success("C4.2", "Desliguei alarme");
    so_debug(">");
}

/**
 * @brief c5_MainCliente Ler a descrição da tarefa C5 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c5_MainCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    MsgContent resposta;

    while (1) {
        ssize_t r = msgrcv(msgId, &resposta, sizeof(MsgContent) - sizeof(long), getpid(), 0);
        if (r == -1) {
            if (errno == EINTR) continue;  // interrupção por sinal
            so_error("C5", "Erro ao ler mensagem na fase de acompanhamento");
            exit(EXIT_FAILURE);
        }

        // Copiar para o pclientRequest, como o validador espera
        *pclientRequest = resposta;

        if (resposta.msgData.status == INFO_TARIFA) {
            so_success("C5", "%s", resposta.msgData.infoTarifa);
        }
        else if (resposta.msgData.status == ESTACIONAMENTO_TERMINADO) {
            so_success("C5", "Estacionamento terminado");
            exit(0);
        }
    }

    // Nunca chega aqui, mas mantém o debug por consistência
    so_debug("> [*pclientRequest:[...]]");
}

/**
 * @brief  c6_TrataCtrlC Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d, msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", sinalRecebido, msgId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    MsgContent msg;
    msg.msgType = clientRequest.msgData.est.pidServidorDedicado;
    msg.msgData.status = TERMINA_ESTACIONAMENTO;
    msg.msgData.est = clientRequest.msgData.est;
    strcpy(msg.msgData.infoTarifa, "");

    if (msgsnd(msgId, &msg, sizeof(msg.msgData), 0) == -1) {
        so_error("C6", "Erro ao enviar TERMINA_ESTACIONAMENTO");
        exit(EXIT_FAILURE);
    }

    so_success("C6", "Cliente: Shutdown");
    so_debug(">");
}

/**
 * @brief  c7_TrataAlarme Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C7", "Cliente: Timeout");
    exit(0);

    so_debug(">");
}