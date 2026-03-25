#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129633       Nome:Rodrigo Vicente Seixas Robalo
## Nome do Módulo: S2. Script: manutencao.sh
## Descrição/Explicação do Módulo:Este script organiza e valida os registros de estacionamento.
## Primeiro, verifica se os dados no arquivo estacionamentos.txt estão corretos, garantindo que os 
## códigos de país sejam válidos, as matrículas tenham o formato adequado e que a data de saída seja 
## posterior à de entrada. Em seguida, transfere os registros completos para arquivos específicos por 
## mês e ano, incluindo o tempo total de estacionamento em minutos. Se houver erros, exibe 
## mensagens de erro e para a execução. Caso tudo esteja correto, conclui com sucesso, 
## ajudando a manter os registros organizados para futuras consultas.
#####################################################################################


paises="paises.txt"
estacionamentos="estacionamentos.txt"

if [[ ! -f "$paises" ]];then
touch "$paises"
chmod 666 "$paises"
fi
if [[ ! -f "$estacionamentos" ]];then
touch "$estacionamentos"
chmod 666 "$estacionamentos"
fi

obter_regex_pais() {
codigo_pais="$1"
regex=$(awk -F"###" -v pais="$codigo_pais" '$1 == pais {print $3}' "$paises")
if [[ -z "$regex" ]]; then
so_debug "Erro: Código de país inválido ($codigo_pais)"
so_error S2.1 "Código de país inválido: $codigo_pais"
exit 2
fi
echo "$regex"
}

validar_matricula() {
matricula="$1"
pais="$2"
regex=$(obter_regex_pais "$pais") || exit 2
if ! [[ "$matricula" =~ $regex ]]; then
so_debug "Erro: O formato da matrícula não corresponde ao do país indicado ($pais)"
so_error S2.1 "Matrícula inválida para o país $pais: $matricula"
exit 3
fi
}

validar_datas() {
data_entrada="$1"
data_saida="$2"
data_entrada=$(echo "$data_entrada" | sed 's/T\([0-9]\{2\}\)h\([0-9]\{2\}\)/T\1:\2/')
data_saida=$(echo "$data_saida" | sed 's/T\([0-9]\{2\}\)h\([0-9]\{2\}\)/T\1:\2/')
tempo_entrada=$(date -d "$data_entrada" +%s)
if [[ -z "$tempo_entrada" ]]; then
so_debug "Erro: Formato de data inválido ($data_entrada)"
so_error S2.1 "Formato de data inválido: $data_entrada"
exit 5
fi
tempo_saida=$(date -d "$data_saida" +%s)
if [[ -z "$tempo_saida" ]]; then
so_debug "Erro: Formato de data inválido ($data_saida)"
so_error S2.1 "Formato de data inválido: $data_saida"
exit 5
fi
if [[ "$tempo_saida" -le "$tempo_entrada" ]]; then
so_debug "Erro: A data de saída é anterior ou igual à data de entrada"
so_error S2.1 "Data de saída ($data_saida) inválida para a data de entrada ($data_entrada)"
exit 4
fi
echo $(( (tempo_saida - tempo_entrada) / 60 ))
}
while IFS=: read -r matricula pais categoria nome data_entrada data_saida; do
if [[ -z "$matricula" || -z "$pais" ]]; then
continue
fi

if [[ -z "$pais" ]]; then
so_debug "Erro: O campo país está vazio na linha lida"
so_error S2.1 "Ficheiro estacionamentos.txt contém linhas mal formatadas"
exit 6
fi
if ! validar_matricula "$matricula" "$pais"; then
so_debug "Matrícula não correspondente ao país" 
so_error S2.1 "Matrícula num formato inválido para o país correspondente"
exit 2
fi
if [[ -n "$data_saida" ]]; then
if ! validar_datas "$data_entrada" "$data_saida"; then
so_debug "Erro: As datas de entrada e saída não foram aprovadas o que significa que a data de saida foi antes da entrada o que é impossível"
so_error S2.1 "Data de saída menor que a data de entrada"
exit 3
fi
fi
done < "$estacionamentos"

so_debug "Código do país, data e matrícula validados com sucesso"
so_success S2.1

while IFS=: read -r matricula pais categoria nome data_entrada data_saida; do
if [[ -n "$data_saida" ]]; then
tempo_estacionamento=$(validar_datas "$data_entrada" "$data_saida")
ano=$(echo "$data_saida" | cut -d'-' -f1)
mes=$(echo "$data_saida" | cut -d'-' -f2)
arquivo="arquivo-${ano}-${mes}.park"

echo "$matricula:$pais:$categoria:$nome:$data_entrada:$data_saida:$tempo_estacionamento" >> "$arquivo"
so_debug "Registo movido para $arquivo"

grep -v "^$matricula:$pais:$categoria:$nome:$data_entrada:$data_saida$" "$estacionamentos" > temp && mv temp "$estacionamentos"
fi
done < "$estacionamentos"

if [[ $? -ne 0 ]]; then
so_debug "Erro inesperado nas permissões de escrita durante o processamento da S2.2"
so_error S2.2 "Erro inesperado"
exit 1
fi
if [[ ! -w "." ]]; then
so_debug "Erro: Sem permissões de escrita na diretoria atual"
so_error S2.2 "Sem permissões de escrita na diretoria"
exit 1
fi
so_debug "Processamento concluído: registos movidos para o mês e ano respetivos e ficheiros atualizados"
so_success S2.2