##  Autor

* **Luis Filipe de Melo Nogueira** - [filipemelonogueira@gmail.com](filipemelonogueira@gmail.com)

# Sistema de Automação de Bomba D'água com ESP32 e Painel Web

Este projeto  de automação e monitoramento de bomba d'água, composto por duas partes principais:

1.  **Firmware para ESP32:** O cérebro do sistema, responsável por ler o sensor de nível, controlar a bomba e se comunicar via MQTT.
2.  **Painel de Controle Web:** Um servidor Node.js que fornece uma interface web amigável para monitorar e controlar o ESP32 em tempo real.

---

Este repositório contém o código-fonte do firmware para um sistema embarcado de automação e monitoramento de bomba d'água, desenvolvido para a plataforma ESP32.

O segundo repositório contém o servidor Web [https://github.com/FilipeMelo07/sistema_bomba_de_agua_node.git](https://github.com/FilipeMelo07/sistema_bomba_de_agua_node.git)

#  Firmware de Controle de Bomba D'água para ESP32


##  Sobre o Firmware

Este software transforma um ESP32 em um controlador inteligente de bomba d'água. Ele utiliza um sensor ultrassônico para medir o nível do reservatório e se conecta a um broker MQTT para receber comandos e enviar dados de status em tempo real. A lógica foi desenvolvida para ser robusta, eficiente e flexível, permitindo total controle remoto.

##  Funcionalidades do Firmware

* **Conectividade Wi-Fi:** Conecta-se a uma rede Wi-Fi local para acesso à internet.
* **Cliente MQTT:** Comunica-se com um broker MQTT para enviar dados e receber comandos de forma assíncrona.
* **Modo de Operação Duplo:**
    * **Automático:** Controla a bomba com base em limites de distância pré-definidos.
    * **Manual:** Permite que a bomba seja ligada ou desligada por comandos MQTT diretos.
* **Configuração Remota:** Os limites de distância para o modo automático podem ser atualizados em tempo de execução via MQTT, sem necessidade de regravar o firmware.
* **Feedback Visual:** Utiliza LEDs para indicar o status da conexão MQTT, o modo de operação e o estado da bomba.

##  Hardware Necessário

Para montar o circuito, você precisará dos seguintes componentes:

* Microcontrolador **ESP32** (qualquer variante com pinos para protoboard)
* Sensor Ultrassônico **HC-SR04**
* Módulo **Relé de 1 Canal** (5V)
* **LEDs** (3 unidades de cores diferentes)
* **Resistores** (~330Ω para cada LED)
* Protoboard e Fios Jumper

###  Mapa de Pinos

Conecte os componentes ao ESP32 conforme a tabela abaixo:

| Pino (GPIO) | Dispositivo Conectado      |
| :---------- | :------------------------- |
| **GPIO 2** | LED Indicador de Modo      |
| **GPIO 12** | Módulo Relé (IN)           |
| **GPIO 14** | LED de Status MQTT         |
| **GPIO 25** | Sensor HC-SR04 (Echo)      |
| **GPIO 26** | Sensor HC-SR04 (Trig)      |
| **GPIO 27** | LED de Status do Relé      |

### Diagrama eletrônico
![](/img/diagrama.png)

##  Guia de Instalação e Execução

Este tutorial guiará você desde o clone do repositório até a conexão do seu ESP32 com o broker MQTT.

### **Pré-requisitos**

1.  **Ambiente ESP-IDF Configurado:** Você precisa ter o framework de desenvolvimento da Espressif (**ESP-IDF**) instalado e configurado corretamente em sua máquina.

2.  **Broker MQTT:** Você precisa de acesso a um broker MQTT. Pode ser um broker local (como o **Mosquitto**) ou um serviço em nuvem. Anote o endereço (URL ou IP) do seu broker.

3.  **Cliente MQTT (Opcional, para testes):** É altamente recomendado ter um cliente MQTT como o [MQTT Explorer](http://mqtt-explorer.com/) para visualizar as mensagens e testar os comandos.

### **Passo 1: Clonar o Repositório**

Abra seu terminal e clone este repositório para sua máquina local:

git clone [https://github.com/FilipeMelo07/sistema_bomba_de_agua.git](https://github.com/FilipeMelo07/sistema_bomba_de_agua.git)

Abra o projeto com o VsCode

### Passo 2: Configurar o Projeto

O ESP-IDF usa um sistema de configuração baseado em menus (menuconfig). É aqui que você definirá suas credenciais de Wi-Fi e o endereço do broker.

Navegue até a pasta do projeto e abra o menu de configuração:

na parte inferior da tela abra o menuconfig

No menu, navegue até a seção Example Connection Configuration.

Configure o WiFi SSID (nome da sua rede Wi-Fi).

Configure o WiFi Password (senha da sua rede Wi-Fi).

Agora, navegue até a seção Example Configuration.

Configure o Broker URL. Insira o endereço completo do seu broker MQTT.

Exemplo local: **mqtt://192.168.1.10**


Salve as configurações e saia do menuconfig.

### Passo 3: Compilar e Gravar o Firmware
Agora que o projeto está configurado, vamos compilar o código e enviá-lo para o ESP32.

Conecte o ESP32 ao seu computador via USB.

no menu inferior

escolha a porta que o seu esp está conectado, (exemplo:**com4**)

click em **ESP-IDF: Build, flash and Monitor**

### Passo 4: Verificar a Conexão
Se tudo correu bem, o monitor s  começará a exibir os logs de inicialização do ESP32.

Você deverá ver as seguintes mensagens de sucesso:

Logs indicando a conexão bem-sucedida com a sua rede Wi-Fi e a obtenção de um endereço IP.

Logo em seguida, a mensagem: MQTT CONECTADO.

Neste momento, o LED de status MQTT (conectado ao GPIO 14) deverá acender, confirmando visualmente que a conexão foi estabelecida com sucesso.

Seu ESP32 está agora online, executando o firmware, conectado ao broker e pronto para enviar dados do sensor e receber comandos! Você pode usar o MQTT Explorer para se inscrever nos tópicos de status e ver as mensagens chegando.

###  Referência dos Tópicos MQTT
Este firmware interage com os seguintes tópicos:

Tópico (Topic)	Ação (O que o firmware faz)

* esp32/comando/modo	[INSCREVE-SE] Ouve por comandos para mudar o modo.
* esp32/comando/rele	[INSCREVE-SE] Ouve por comandos para ligar/desligar.
* esp32/comando/dist_ligar	[INSCREVE-SE] Ouve por novos valores para o limite de ligar.
* esp32/comando/dist_desligar	[INSCREVE-SE] Ouve por novos valores para o limite de desligar.
* esp32/status/modo	[PUBLICA] Envia seu modo de operação atual.
* esp32/status/bomba	[PUBLICA] Envia o estado atual da bomba.
* esp32/status/distancia	[PUBLICA] Envia a distância medida pelo sensor.



##  Painel de Controle Web (Servidor Node.js)

Para controlar e monitorar o ESP32 de forma amigável, este projeto inclui um servidor web leve, construído com **Node.js** e **Express**. Ele atua como uma ponte entre o usuário (através do navegador) e o dispositivo (via MQTT), fornecendo uma interface gráfica em tempo real.

###  Funcionalidades do Painel

* **Interface Web Responsiva:** Acesse o painel de qualquer dispositivo com um navegador, seja um computador ou smartphone.
* **Visualização em Tempo Real:** Veja o status atual da bomba, o modo de operação e a distância medida pelo sensor, tudo atualizado automaticamente sem precisar recarregar a página.
* **Controle Total:** Altere o modo de operação entre **Automático** e **Manual**.
* **Comandos Manuais:** Ligue e desligue a bomba diretamente pela interface (quando em modo manual).
* **Configuração Remota:** Ajuste e envie os limites de distância para ligar e desligar a bomba no modo automático.

###  Guia de Instalação do Painel

Siga os passos abaixo para rodar o servidor do painel de controle na sua máquina local.

#### **Pré-requisitos**

1.  **Node.js e npm:** Você precisa ter o [Node.js](https://nodejs.org/) instalado. O npm (Node Package Manager) já vem incluído. Para verificar se estão instalados, abra um terminal e digite `node -v` e `npm -v`.


#### **Passo 1: Preparar o Projeto**

Abra um terminal e clone este repositório para sua máquina:

git clone [https://github.com/FilipeMelo07/sistema_bomba_de_agua_node.git](https://github.com/FilipeMelo07/sistema_bomba_de_agua_node.git)

2.  Abra um terminal na mesma pasta do projeto.

### **Passo 2: Instalar as Dependências**

O servidor depende de duas bibliotecas principais: `express` e `mqtt`. Execute o comando abaixo para instalá-las:

```bash
npm install express mqtt
```
Este comando criará uma pasta `node_modules` e um arquivo `package-lock.json`.

#### **Passo 3: Iniciar o Broker MQTT**

Se o seu broker MQTT (como o Mosquitto) não estiver rodando, inicie-o agora em um terminal separado.

#### **Passo 4: Rodar o Servidor**

Agora que as dependências estão instaladas e o broker está online, inicie o servidor Node.js com o seguinte comando:

```bash
node servidor.js
```

Se tudo ocorrer bem, você verá as seguintes mensagens no terminal:

```
Conectado com sucesso ao Broker MQTT em: mqtt://localhost
Inscrito nos tópicos de status do ESP32!
Servidor web iniciado! Acesse em http://localhost:3000
```

#### **Passo 5: Acessar o Painel**

Abra seu navegador de internet e acesse o endereço:

**[http://localhost:3000](http://localhost:3000)**

Pronto! Agora você pode ver os dados enviados pelo seu ESP32 em tempo real e enviar comandos para controlar a bomba d'água diretamente da página web.

---


