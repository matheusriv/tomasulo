# Processador Superescalar com Tomasulo

Este projeto implementa um simulador de um processador superescalar utilizando o algoritmo de Tomasulo.

## Pré-requisitos

Para compilar e executar o projeto, você precisa ter o **SystemC** instalado no seu sistema.

## Configuração

Antes de compilar, é necessário configurar a variável de ambiente `SYSTEMC_PATH` para que ela aponte para o diretório de instalação do SystemC.

Exemplo num sistema baseados em Unix:
```sh
export SYSTEMC_PATH="/usr/local/systemc"
```
## Testando o Processador
O processador se encontra em `src/Processor.cpp`. 

Compile o código usando o comando make:
```sh
make
```

Para rodar o teste, executando com um arquivo com instruções, siga o exemplo:
```sh
./build/tomasulo programs/teste1.txt
```