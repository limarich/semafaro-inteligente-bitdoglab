# SEMÁFORO-INTELIGENTE-BITDOGLAB

## 🎯 Objetivo Geral

Desenvolver um semáforo inteligente e acessível utilizando os periféricos da placa **BitdogLab** e o sistema operacional de tempo real **FreeRTOS**. O sistema simula os estados de um semáforo tradicional (verde, amarelo, vermelho) e um modo noturno intermitente, com feedbacks visuais e sonoros, promovendo acessibilidade. O projeto tem como foco a aplicação de multitarefas e gerenciamento concorrente em sistemas embarcados.

---

## ⚙️ Descrição Funcional

O sistema é composto por quatro estados de operação:

| Estado   | Tempo Visual    | Feedback Sonoro            |
| -------- | --------------- | -------------------------- |
| Verde    | 3 segundos      | Beep contínuo de 1 segundo |
| Amarelo  | 1,5 segundos    | Beeps intermitentes        |
| Vermelho | 4 segundos      | Beep de 0,5 segundos       |
| Noturno  | Pisca 1s ON/OFF | Beep a cada 2 segundos     |

A operação do sistema é gerenciada por **seis tarefas (tasks)** que rodam em paralelo sob o controle do FreeRTOS:

### 🔹 `vTrafficLightControllerTask`

Gerencia a lógica central de transição entre os estados do semáforo, controlando o tempo de cada fase. Um estado global (enum) é compartilhado entre as tasks.

### 🔹 `vLedTask`

Controla o LED RGB para refletir o estado atual do semáforo. Em modo noturno, pisca em amarelo.

### 🔹 `vTrafficLightTask`

Desenha o semáforo em uma matriz de LEDs, com animações diferentes para cada estado.

### 🔹 `vBuzzerTask`

Controla os buzzers com sons distintos para cada estado. Pode ser desligado pelo botão B.

### 🔹 `vButtonTask`

Lê os botões físicos:

- **Botão A** alterna entre o modo normal e o modo noturno.
- **Botão B** liga/desliga o feedback sonoro (buzzer).

### 🔹 `vDisplayAnimationTask`

Exibe uma animação de boneco no display OLED SSD1306, com mensagens informativas dependendo do estado atual:

| Estado   | Mensagem no Display |
| -------- | ------------------- |
| Verde    | "PODE SEGUIR!"      |
| Amarelo  | "ATENÇÃO!"          |
| Vermelho | "ESPERE!"           |
| Noturno  | "ATENÇÃO!" (pisca)  |

---

## 🧩 Periféricos Utilizados (BitDogLab)

| Periférico         | Função Principal                             |
| ------------------ | -------------------------------------------- |
| **Botões**         | Alternam modos e controlam buzzer            |
| **Matriz de LEDs** | Exibe o semáforo com animações               |
| **LED RGB**        | Indica visualmente os estados do semáforo    |
| **Buzzers**        | Fornecem feedback sonoro para acessibilidade |
| **Display OLED**   | Animação e mensagens visuais informativas    |

---

## 🧠 Tecnologias e Bibliotecas

- **C/C++**
- **FreeRTOS**
- **Pico SDK**
- **PIO (Programmable I/O)**
- **Drivers personalizados para matriz de LED, SSD1306, e buzzers**

---

## 🧪 Como Executar

1. Compile o código usando o ambiente configurado com a biblioteca **Raspberry pi pico** no seu **VSCode**.
2. Compile o código para a sua **BitdogLab**.
3. Pressione **Botão A** para alternar entre os modos normal e noturno.
4. Pressione **Botão B** para ativar ou desativar o som dos buzzers.

---

## 📂 Estrutura do Projeto

```
├── main.c
├── lib/
│ ├── buzzer.h / .c
│ ├── leds.h / .c
│ ├── ssd1306.h / .c
│ └── font.h
├── pio_matrix.pio
├── README.md
```

---

## 📺 Demonstração

- **Vídeo de demonstração**:

---

## 🧑‍💻 Autor

Richard Lima Ribeiro  
Curso de Sistemas Embarcados  
Polo Juazeiro

---
