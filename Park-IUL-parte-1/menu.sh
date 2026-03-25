#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129633       Nome:Rodrigo Vicente Seixas Robalo
## Nome do Módulo: S4. Script: menu.sh
## Descrição/Explicação do Módulo: Este script funciona como um menu interativo para 
## chamar os outros scripts do sistema de estacionamento Park-IUL. O utilizador escolhe 
## uma opção escrevendo o respetivo número, e o script valida essa entrada.
## Se a opção for 1 ou 2, o script pede informações como matrícula, país e categoria antes
## de invocar regista_passagem.sh para registar a entrada ou saída da viatura, respetivamente.
## Se a opção for 3, o script invoca manutencao.sh para realizar a manutenção.
## Se a opção for 4, o utilizador deve escolher quais as estatísticas que deseja visualizar e o 
## script invoca stats.sh com os parâmetros apropriados.
## Se a opção for 0, o script exibe "Sair" e termina.
## Qualquer entrada inválida exibe um erro e volta ao menu.
#####################################################################################

while true; do
    echo "MENU:"
    echo "1: Regista passagem - Entrada estacionamento"
    echo "2: Regista passagem - Saída estacionamento"
    echo "3: Manutenção"
    echo "4: Estatísticas"
    echo "0: Sair"
    echo ""
    echo -n "Opção: "
    read opcao

    case $opcao in
        1)
            so_success S4.2.1 $opcao
            read -p "Indique a matrícula da viatura: " matricula
            read -p "Indique o código do país de origem da viatura: " pais
            read -p "Indique a categoria da viatura [L(igeiro)|P(esado)|M(otociclo)]: " categoria
            read -p "Indique o nome do condutor da viatura: " nome
            ./regista_passagem.sh "$matricula" "$pais" "$categoria" "$nome"
            so_success S4.3 "Entrada estacionamento registada com sucesso."
            ;;
        2)
            so_success S4.2.1 $opcao
            echo "Regista passagem de Saída do estacionamento:" 
            read -p "Indique a matrícula da viatura: " matricula
            read -p "Indique o código do país de origem da viatura: " pais
            ./regista_passagem.sh "$pais/$matricula"
            so_success S4.4 "Saída estacionamento registada com sucesso"
            ;;
        3)
            so_success S4.2.1 $opcao
            ./manutencao.sh
            so_success S4.5 "Manutenção concluída"
            ;;
        4)
            so_success S4.2.1 $opcao
            echo "Estatísticas:"
            echo "1: matrículas e condutores cujas viaturas estão ainda estacionadas no parque"
            echo "2: top3 das matrículas das viaturas que passaram mais tempo estacionadas"
            echo "3: tempos de estacionamento de ligeiros e pesados agrupadas por país"
            echo "4: top3 das matrículas das viaturas que estacionaram mais tarde num dia"
            echo "5: tempo total de estacionamento por utilizador"
            echo "6: matrículas e tempo total de estacionamento delas, agrupadas por país da matrícula"
            echo "7: top3 nomes de condutores mais compridos"
            echo "8: todas as estatísticas anteriores, na ordem numérica indicada"
            echo ""
            read -p "Indique quais as estatísticas a incluir, opções separadas por espaço: " estatistica
            
            case $estatistica in
                "1 2 3 4 5 6 7 8"|8)
                    ./stats.sh
                    so_success S4.6
                    ;;
                    "")
                    so_error S4.6 $estistica
                    continue
                    ;;
                *)
                    ./stats.sh $estatistica
                    so_success S4.6
                    ;;
            esac
            ;;
        0)
                so_success S4.2.1 0
                echo "Sair"
                exit 0
                ;;
        *)
                so_error S4.2.1 $opcao
                ;;
        esac 
done
