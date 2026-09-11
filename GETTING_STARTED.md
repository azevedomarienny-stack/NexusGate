# Como executar o NexusGate

Guia passo a passo para clonar, compilar e rodar o projeto do zero no Windows.

## Pré-requisitos

Você vai precisar instalar 3 coisas: Git, MinGW-w64 (compilador C/C++) e Python.

### 1. Git

Baixe e instale em: https://git-scm.com/download/win

### 2. MinGW-w64 (via MSYS2)

1. Baixe e instale o MSYS2: https://www.msys2.org/
2. Abra o terminal **"MSYS2 UCRT64"** (procure no menu Iniciar — não use o "MSYS2 MSYS" comum) e rode:
```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb
```
3. Adicione a pasta abaixo ao PATH do Windows:

C:\msys64\ucrt64\bin

   (Painel de Controle → Sistema → Configurações avançadas → Variáveis de Ambiente → selecione "Path" → Editar → Novo → cole o caminho acima)
4. Abra um **novo** terminal PowerShell (o PATH só atualiza em terminais novos) e confirme:
```powershell
   g++ --version
   gcc --version
```
   Se ambos mostrarem uma versão, está pronto.

### 3. Python 3

Baixe em: https://www.python.org/downloads/ (marque a opção "Add Python to PATH" durante a instalação).

Confirme com:
```powershell
python --version
```

---

## Passo 1 — Clonar o projeto

```powershell
git clone https://github.com/SEU-USUARIO/nexusgate.git
cd nexusgate
```

## Passo 2 — Compilar o SQLite (uma única vez)

```powershell
mkdir build
gcc -c third_party/sqlite3.c -o build/sqlite3.o -O2
```
Essa etapa demora um pouco (o SQLite é um arquivo grande) — é normal.

## Passo 3 — Compilar o NexusGate

```powershell
g++ -std=c++20 -Iinclude -Ithird_party src/main.cpp src/Socket.cpp src/TcpListener.cpp src/ThreadPool.cpp src/BackendPool.cpp src/Logger.cpp src/Metrics.cpp src/HttpMetricsServer.cpp src/Database.cpp src/RateLimiter.cpp build/sqlite3.o -o build/nexusgate.exe -lws2_32
```

Se não aparecer nenhuma mensagem de erro, o executável foi criado em `build/nexusgate.exe`.

## Passo 4 — Subir os backends de teste

Abra **dois terminais novos** (mantenha-os abertos):

Terminal A:
```powershell
python tools/fake_backend.py 9001
```

Terminal B:
```powershell
python tools/fake_backend.py 9002
```

## Passo 5 — Rodar o NexusGate

Em um **terceiro terminal**:
```powershell
.\build\nexusgate.exe
```

Você deve ver:

[INFO ] NexusGate escutando na porta 8080 com 4 workers...
[INFO ] Servidor de metricas escutando na porta 8081


## Passo 6 — Ver o dashboard

```powershell
start web/dashboard.html
```

## Passo 7 — Gerar tráfego de teste (opcional)

Em um **quarto terminal**:
```powershell
$client = New-Object System.Net.Sockets.TcpClient("localhost", 8080)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$writer.WriteLine("teste")
$writer.Flush()
$reader = New-Object System.IO.StreamReader($stream)
$reader.ReadLine()
$client.Close()
```

Rode algumas vezes e observe o dashboard atualizando com o tráfego.

## Rodando os testes automatizados

```powershell
g++ -std=c++20 -Iinclude tests/test_runner.cpp src/BackendPool.cpp src/RateLimiter.cpp src/Metrics.cpp -o build/tests.exe
.\build\tests.exe
```

## Rodando o teste de carga

```powershell
python tools/load_test.py
```

## Problemas comuns

| Sintoma | Causa provável |
|---|---|
| `g++`/`gcc` não reconhecido | PATH não configurado ou terminal aberto antes da configuração — abra um novo terminal |
| `bind falhou (erro 10048)` | Porta 8080 ou 8081 já em uso — feche outra instância do NexusGate rodando |
| Dashboard não atualiza | Confirme que o `nexusgate.exe` está rodando e que a chave de API no `dashboard.html`/`script.js` bate com a esperada no servidor |
| `undefined reference to sqlite3_...` | Esqueceu de compilar/incluir `build/sqlite3.o` no comando de build |