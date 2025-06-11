# Preparando o ambiente de programação

## 1º passo: Instalação do Git

A instalação do git é necessária para clonar o repositório do pico-sdk
e do picotool.

Primeiro precisamos atualizar o sistema, para isso, abra o terminal
(atalho: Ctrl+alt+t) e digite o seguinte comando:

```
sudo apt update && sudo apt upgrade -y
```

Depois de executar as atualizações, continue no terminal para instalar o Git:

```
sudo apt install git -y
```

Após a instalação, digite o comando abaixo para verificar se realmente foi instalado e também a versão:

```
git --version
```

## 2º passo: Instalação do pico-sdk

Agora que o Git está instalado, é hora de fazer o clone do pico-sdk e suas devidas instalações.

Primeiro crie um diretório para armazenar o pico-sdk. Abra o terminal e digite os seguintes comandos:

```
mkdir pico
```

Obs: o comando mkdir serve para criar uma nova pasta, e o nome que vem posteriormente é o nome que será dado a essa pasta, no exemplo será a pasta pico.

Agora, selecione a pasta que você acabou de criar:

```
cd pico
```

Com a pasta selecionada, agora vamos fazer o clone do repositório do pico-sdk:

```
git clone -b master https://github.com/raspberrypi/pico-sdk.git
```

Após concluir a clonagem do repositório, selecione o diretório do
pico-sdk:

```
cd pico-sdk
```

Inicialize os submódulos:

```
git submodule update --init
```

## 3º passo: Instalação do picotoo

Além do pico-sdk, também é necessário baixar e configurar o
picotool, para isso, continue no terminal.

Antes de iniciar o próximo passo, é importante voltar para a pasta
raiz, para isso, digite o comando:

```
cd ~/
```

Agora, vamos instalar as dependências do ARM GCC:

```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi buildessential -y
```

Depois de instalar as dependências do ARM GCC, vamos voltar para o
diretório onde estamos instalando as bibliotecas, para isso,
selecione a pasta que criamos anteriormente (pico):

```
cd pico
```

Após selecionar a pasta, vamos fazer o clone do repositório do
picotool:

```
git clone https://github.com/raspberrypi/picotool.git
```

Selecione a pasta do picotool:

```
cd picotool
```

Depois vamos criar uma outra pasta:

```
mkdir build
```

Agora vamos voltar para o diretório raiz:

```
cd ~/
```

Para configurar o cmake, antes precisamos configurar alguns
caminhos para que o linux os reconheça na instalação.

Procure o caminho para a pasta pico-sdk no seu dispositivo e copie.


*Imagem aqui*

Digite no terminal:

```
export PICO_SDK_PATH=cole-o-caminho-aqui
```

Obs: Copie o caminho para a pasta do pico-sdk do seu dispositivo
e cole no lugar de “```cole-o-caminho-aqui```”

Para encontrar o caminho da pasta, acesse com o terminal até
entrar na sua pasta pico-sdk com os comandos cd. Depois que
estiver na pasta do pico-sdk, digite o comando pwd e copie o
caminho.

Ex.: *imagem aqui*

Após copiar o caminho volte para a pasta raiz:

```
cd ~/
```

Ainda na pasta raiz digite:

```
echo 'export PICO_SDK_PATH=/home/nomeDeUsuario/pico/pico-sdk'>> ~/.bashrc
```

Para salvar as alterações, digite:

```
source ~/.bashrc
```

Com o caminho configurado, vamos voltar para a pasta do picotool.
Para isso, digite:

```
cd pico
```

Depois:

```
cd picotool
```

Depois selecione a pasta que criamos anteriormente:

```
cd build
```

Depois vamos configurar o cmake:

```
cmake ..
```

Agora digite:

```
make
```

Instale o picotool:

```
sudo cp picotool /usr/local/bin
```

Por último, verifique se o picotool está configurado corretamente:

```
picotool version
```

