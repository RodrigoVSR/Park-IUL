/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Este Módulo deverá ser alterado para definir IPC_KEY, mas não precisa ser entregue
 ** Nome do Módulo: defines.h
 ** Descrição/Explicação do Módulo:
 **     Definição do valor único IPC_KEY para o trabalho
 **
 ***************************************************************************************/
#ifndef __DEFINES_H__
    #define __DEFINES_H__

    #include "common.h"

    /**
    * KEY to be used on all Linux IPC get operations
    */
    #define IPC_KEY 0x0a129633      // Suggestion: Replace with your student number

#endif // __DEFINES_H__