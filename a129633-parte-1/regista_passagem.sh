#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129633       Nome:Rodrigo Vicente Seixas Robalo
## Nome do Módulo: S1. Script: regista_passagem.sh
## Descrição/Explicação do Módulo: Este script regista a entrada e saída de viaturas no estacionamento,validando os dados
## antes de atualizar os registos. Primeiro, verifica o número de argumentos passados e 
## identifica se a ação é uma entrada ou saída. Em seguida, valida a matrícula com base
## no código do país, consultando o ficheiro paises.txt. Se for uma entrada, confirma que o
## nome do condutor existe no sistema e que a viatura ainda não está estacionada. Para saídas,
## certifica-se de que há um registo de entrada e que a viatura ainda não saiu. Depois da validação,
## o script atualiza a passagem no ficheiro **estacionamentos.txt**. Por fim, ordena
## os registos por hora de entrada e gera **estacionamentos-ordenados-hora.txt**. Em caso de erro,
## exibe mensagens e termina a execução.
#####################################################################################

#!/bin/bash

if [[ $# -eq 1 ]]; then
    acao="saida"
    IFS='/' read -r pais matricula <<< "$1"
    if [[ -z "$pais" || -z "$matricula" ]]; then
        so_debug "Erro: Formato de saída inválido. Esperado: pais/matricula"
        so_error S1.1 "Formato de saída inválido"
        exit 1
    fi
elif [[ $# -eq 4 ]]; then
    acao="entrada"
    matricula="$1"
    pais="$2"
    categoria="$3"
    nomeCondutor="$4"
    if [[ ! "$categoria" =~ ^(L|P|M)$ ]]; then
        so_debug "Erro: Categoria inválida ($categoria), deve ser L, P ou M"
        so_error S1.1 "Categoria incorreta"
        exit 1
    fi
else
    so_debug "Erro: Número de argumentos inválido"
    so_error S1.1 "Número de argumentos inválido"
    exit 1
fi

case $pais in
    PT) padrao='^[A-Z]{2}[ -]?[0-9]{2}[ -]?[A-Z]{2}$' ;;
    ES) padrao='^[0-9]{4}[ -]?[B-Z]{3}$' ;;
    FR) padrao='^[A-Z]{2}[ -]?[0-9]{3}[ -]?[A-Z]{2}$' ;;
    UK) padrao='^[A-Z]{2}[0-9]{2}[ ]?[A-Z]{3}$' ;;
    *)  so_debug "Erro: Código de país inválido ($pais)"
        so_error S1.1 "Código de país inválido"
        exit 1 ;;
esac

if ! [[ "$matricula" =~ $padrao ]]; then
    so_debug "Erro: Matrícula inválida para o país $pais ($matricula)"
    so_error S1.1 "Matrícula inválida para $pais"
    exit 1
fi

Utilizadores_Tigre="_etc_passwd"
nomes=$(echo "$nomeCondutor" | wc -w)
if [[ "$acao" == "entrada" && $nomes -ne 2 ]]; then
    so_debug "Erro: Nome do condutor deve ter apenas primeiro e último nome"
    so_error S1.1 "Nome inválido"
    exit 1
fi

Primeiro_Nome=$(awk '{print $1}' <<< "$nomeCondutor")
Ultimo_Nome=$(awk '{print $NF}' <<< "$nomeCondutor")

if ! grep -i -q "^[^:]*:[^:]*:[^:]*:[^:]*:$Primeiro_Nome.*$Ultimo_Nome,.*$" "$Utilizadores_Tigre"; then
    so_debug "Erro: Nome do condutor não encontrado no sistema"
    so_error S1.1 "Nome do condutor não encontrado"
    exit 1
fi

so_success S1.1

estacionamentos_file="estacionamentos.txt"
[[ ! -f "$estacionamentos_file" ]] && touch "$estacionamentos_file" && chmod 666 "$estacionamentos_file"

matriculaLimpa=$(echo "$matricula" | sed 's/[^a-zA-Z0-9]//g')
registos=$(grep "$matriculaLimpa" "$estacionamentos_file")

total_entradas=$(echo "$registos" | awk -F: '{if ($5 != "") count++} END {print count}')
total_saidas=$(echo "$registos" | awk -F: '{if ($6 != "") count++} END {print count}')

if [[ "$acao" == "entrada" ]]; then
    if [[ "$total_entradas" -gt "$total_saidas" ]]; then
        so_debug "Erro: Viatura já está no parque"
        so_error S1.2 "Entrada duplicada"
        exit 1
    fi
    so_debug "Viatura registada com sucesso"
    so_success S1.2
fi

if [[ "$acao" == "saida" ]]; then
    if [[ "$total_entradas" -eq "$total_saidas" ]]; then
        so_debug "Erro: Nenhuma entrada correspondente encontrada"
        so_error S1.2 "Viatura não encontrada no parque"
        exit 1
    fi
    so_debug "Saída registada com sucesso"
    so_success S1.2
fi

[[ ! -w "$estacionamentos_file" ]] && so_debug "Erro: Ficheiro sem permissões de escrita" && so_error S1.3 "Ficheiro sem permissões" && exit 1

DataHora=$(date '+%Y-%m-%dT%Hh%M')
ultima_linha=$(grep "$matriculaLimpa" "$estacionamentos_file" | tail -1)

if [[ "$acao" == "entrada" ]]; then
    echo "$matriculaLimpa:$pais:$categoria:$nomeCondutor:$DataHora" >> "$estacionamentos_file"
    so_debug "Entrada registada na base de dados"
    so_success S1.3
elif [[ "$acao" == "saida" ]]; then
    [[ -z "$ultima_linha" ]] && so_debug "Erro: Nenhuma entrada encontrada" && so_error S1.3 "Nenhuma entrada encontrada" && exit 1
    sed -i "s|$ultima_linha|$ultima_linha:$DataHora|" "$estacionamentos_file"
    so_debug "Saída registada na base de dados"
    so_success S1.3
fi

sort -t':' -k5.12,5.16 "$estacionamentos_file" > estacionamentos-ordenados-hora.txt
so_debug "Ficheiro de estacionamentos ordenado"
so_success S1.4
