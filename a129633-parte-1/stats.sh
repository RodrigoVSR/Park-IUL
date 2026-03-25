#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129633       Nome:Rodrigo Vicente Seixas Robalo
## Nome do Módulo: S3. Script: stats.sh
## Descrição/Explicação do Módulo: Este script gera estatísticas sobre os estacionamentos do 
## Park-IUL e as salva no ficheiro stats.html. 
## Primeiro, verifica a existência e acessibilidade dos arquivos necessários. Se houver erro,
## exibe so_error e encerra. Caso contrário, cria stats.html com a data e hora atuais e com a 
## mesma estrutura de stats-exemplo.html Dependendo dos argumentos passados, processa estatísticas
## específicas usando awk, sort e sed, extraindo e formatando os dados.
## As stats incluem nomes dos condutores, matrículas, tempos de estacionamento e rankings.
#####################################################################################

data_atual=$(date "+%Y-%m-%d")
hora_atual=$(date "+%H:%M:%S")
echo "<html><head><meta charset=\"UTF-8\"><title>Park-IUL: Estatísticas de estacionamento</title></head>" > stats.html
echo "<body><h1>Lista atualizada em $data_atual $hora_atual</h1>" >> stats.html

if ! ls arquivo-*.park 2>/dev/null; then
    so_error "S3.1"
    exit 1
fi
for ficheiro in arquivo-*.park; do
    if [[ ! -r "$ficheiro" ]]; then
        so_error "S3.1"
        exit 1
    fi
done
if [[ ! -f "paises.txt" || ! -r "paises.txt" || ! -f "estacionamentos.txt" || ! -r "estacionamentos.txt" ]]; then
    so_error "S3.1"
    exit 1
fi

if [[ $# -eq 0 ]]; then
    set -- 1 2 3 4 5 6 7 
fi

for arg in "$@"; do #@ percorre os arg um a um. Ao contrário do *, que os junta numa string.
    if ! [[ "$arg" =~ ^[1-7]$ ]]; then 
        so_error "S3.1"
        exit 1
    fi
done
so_success S3.1

if [[ ! -f "estacionamentos.txt" ]]; then
    so_error "S3.2 estacionamentos.txt não encontrado"
fi

for arg in "$@"; do
    case "$arg" in
1) 
carspark=$(awk -F':' 'NF==5 {print $4 "###" $1}' "estacionamentos.txt" | sed '/^###/d' | sort)
echo "<h2>Stats1:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS= read -r line; do
    nome=$(echo "$line" | awk -F'###' '{print $1}')
    matricula=$(echo "$line" | awk -F'###' '{print $2}')
    echo "<li><b>Matrícula:</b> $matricula <b>Condutor:</b> $nome</li>" >> stats.html
done <<< "$carspark"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
2)
maistempo=$(awk -F':' '{tempo[$1]+=$NF} END {for (mat in tempo) print mat "|" tempo[mat]}' arquivo-*.park | sort -t'|' -k2,2nr | head -n 3)
echo "<h2>Stats2:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS= read -r line; do
    Matricula=$(echo "$line" | cut -d'|' -f1)
    TempoParkMinutos=$(echo "$line" | cut -d'|' -f2)
    echo "<li><b>Matrícula:</b> $Matricula <b>Tempo estacionado:</b> $TempoParkMinutos</li>" >> stats.html
done <<< "$maistempo"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
3)
pais=$(awk -F':' '$3 != "M" {time[$2] += $NF} END {for (pais in time) print pais "|" time[pais]}' arquivo-*.park | sort -t'|' -k2,2nr -k1,1)
echo "<h2>Stats3:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS= read -r line;do
    NomePais=$(echo "$line" | cut -d'|' -f1)
    TempoParkMinutos=$(echo "$line" | cut -d'|' -f2)
    NomePais=$(awk -F'###' -v code="$NomePais" '$1 == code {print $2}' paises.txt)
    echo "<li><b>País:</b> $NomePais <b>Total tempo estacionado:</b> $TempoParkMinutos</li>" >> stats.html
done <<< "$pais"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
4)
maistempo3=$(awk -F':' '{split($5, a, "T"); print $1 "|" $2 "|" $5 "|" a[2]}' estacionamentos.txt arquivo-*.park | sort -t'|' -k4,4r | head -n 3 | sort -t'|' -k4,4 | cut -d'|' -f1-3)
echo "<h2>Stats4:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS="|" read -r matricula pais data hora; do
echo "<li><b>Matrícula:</b> $matricula <b>País:</b> $pais <b>Data Entrada:</b> $data$hora</li>" >> stats.html
done <<< "$maistempo3"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
5)
top_condutores=$(awk -F':' '{tempo[$4] += $NF} END {for (c in tempo) print c "|" tempo[c]}' arquivo-*.park | sort -t'|' -k2,2nr -k1,1)
echo "<h2>Stats5:</h2>" >> stats.html
echo "<ul>" >> stats.html
if [[ -z "$top_condutores" ]]; then
    echo "</ul>" >> stats.html
    echo "S3.2.5 Nenhum veículo registado"
    if [[ $# -eq 1 && "$1" == "5" ]]; then
            echo "</body></html>" >> stats.html
    exit 0
    fi
fi
while IFS="|" read -r condutor tempo; do
    dias=$((tempo / 1440))
    horas=$(((tempo % 1440) / 60))
    minutos=$((tempo % 60))
    echo "<li><b>Condutor:</b> $condutor <b>Tempo total:</b> $dias dia(s), $horas hora(s) e $minutos minuto(s)</li>" >> stats.html
done <<< "$top_condutores"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
6)
time=$(awk -F':' '{
    pais = $2;
    matricula = $1;
    tempo = $NF;
    tempo_pais[pais] += tempo;
    tempo_car[pais "|" matricula] += tempo;
} END {
    for (pais in tempo_pais) {
        print pais "|" tempo_pais[pais];
    }
}' arquivo-*.park | sort)

echo "<h2>Stats6:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS="|" read -r pais tempo_total; do
nomepais=$(awk -F'###' -v code="$pais" '$1 == code {print $2}' paises.txt)
    echo "<li><b>País:</b> $nomepais <b>Total tempo estacionado:</b> $tempo_total</li>" >> stats.html
    echo "<ul>" >> stats.html
    matriculas=$(awk -F':' -v p="$pais" '{
        if ($2 == p) {
            pais = $2;
            matricula = $1;
            tempo = $NF;
            tempo_car[matricula] += tempo;
        }
    } END {
        for (matricula in tempo_car) {
            print matricula "|" tempo_car[matricula];
        }
    }' arquivo-*.park | sort)
    while IFS="|" read -r matricula tempo_carro; do
        if [[ -n "$matricula" ]]; then
            echo "<li><b>Matrícula:</b> $matricula <b> Total tempo estacionado:</b> $tempo_carro</li>" >> stats.html
        fi
    done <<< "$matriculas"
    echo "</ul>" >> stats.html #fecha a lista de matriculas, não a principal
done <<< "$time"
echo "</ul>" >> stats.html
echo "" >> stats.html
;;
7)
names=$(awk -F':' '{print $4}' estacionamentos.txt arquivo-*.park | sort -u)
nomes_maiores=$(echo "$names" | awk '{print length, $0}' | sort -k1,1nr -k2 | cut -d ' ' -f2- | head -n 3)
echo "<h2>Stats7:</h2>" >> stats.html
echo "<ul>" >> stats.html
while IFS= read -r line; do
    echo "<li><b> Condutor:</b> $line</li>" >> stats.html
done <<< "$nomes_maiores"   
echo "</ul>" >> stats.html
;;
esac
done
echo "</body></html>" >> stats.html
so_success S3.3
