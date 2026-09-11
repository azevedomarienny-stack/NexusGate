# NexusGate

Load balancer / reverse proxy TCP de alta performance escrito em C++, com balanceamento de carga, health checks automáticos, métricas em tempo real e persistência histórica em SQLite.

> Guia detalhado de instalação e execução: [GETTING_STARTED.md](GETTING_STARTED.md)

## O problema

Sistemas com múltiplos servidores backend precisam de algo que distribua carga de forma inteligente, detecte automaticamente quando um servidor cai, e dê visibilidade sobre a saúde e performance do sistema em tempo real.

## Como o NexusGate resolve isso

- Aceita conexões TCP e distribui entre um pool de backends configuráveis, usando round-robin.
- Executa health checks periódicos em threads dedicadas, removendo/reinserindo backends do pool automaticamente conforme ficam disponíveis ou não.
- Expõe métricas em tempo real (latência, throughput, erros, conexões ativas) via endpoint HTTP protegido por API key.
- Persiste histórico de eventos e snapshots de métricas em SQLite.
- Aplica rate limiting por IP e timeouts de socket para resistir a abuso e conexões penduradas.
- Dashboard web com Chart.js consumindo as métricas via polling.

## Arquitetura
            [ Clientes ]
                 |
    [ NexusGate — Nucleo em C++ ]

```text
      [ NexusGate - Núcleo em C++ ]
  ┌─────────────────────────────────┐

  | TCP Listener (Winsock2)         |
  | Thread Pool (4 workers)         |
  | Rate Limiter (por IP)           |
  | Load Balancer (round-robin)     |
  | Health Check Manager (thread)   |
  | Metrics Collector               |
  | HTTP Server (/metrics, API key) |
  └─────────────────────────────────┘

                | | |
  [Backend 1] [Backend 2] [Backend N]
                 |
   [ SQLite: eventos + snapshots ]
                 |
 [ Dashboard Web (HTML/JS/Chart.js) ]
```



## Tecnologias

- **C++17/20** — núcleo do sistema (proxy, balanceamento, concorrência, gerenciamento de memória via RAII)
- **Winsock2** — sockets TCP nativos do Windows
- **SQLite** (amalgamation) — persistência de eventos e métricas
- **HTML/CSS/JavaScript + Chart.js** — dashboard de observabilidade
- **Python** — backends de teste e script de carga (ferramentas de apoio, fora do núcleo)

## Conceitos demonstrados

Multithreading e thread pool · mutex e condition variables · programação de sockets TCP · RAII e gerenciamento de memória (smart pointers, move semantics) · balanceamento de carga · health checks e tolerância a falhas · rate limiting · timeouts de rede · persistência com prepared statements (SQLite) · arquitetura modular · testes automatizados.

## Requisitos

- Windows com MinGW-w64 (via MSYS2)
- VS Code (opcional, mas recomendado)
- Python 3 (apenas para os backends de teste e load test)

## Instalação e execução

```powershell
# Compilar o SQLite (uma unica vez)
gcc -c third_party/sqlite3.c -o build/sqlite3.o -O2

# Compilar o projeto
g++ -std=c++20 -Iinclude -Ithird_party src/main.cpp src/Socket.cpp src/TcpListener.cpp src/ThreadPool.cpp src/BackendPool.cpp src/Logger.cpp src/Metrics.cpp src/HttpMetricsServer.cpp src/Database.cpp src/RateLimiter.cpp build/sqlite3.o -o build/nexusgate.exe -lws2_32

# Subir backends de teste (dois terminais)
python tools/fake_backend.py 9001
python tools/fake_backend.py 9002

# Rodar o NexusGate
.\build\nexusgate.exe
```

Abra `web/dashboard.html` no navegador para ver o dashboard em tempo real.

## Testes

```powershell
g++ -std=c++20 -Iinclude tests/test_runner.cpp src/BackendPool.cpp src/RateLimiter.cpp src/Metrics.cpp -o build/tests.exe
.\build\tests.exe
```

## Teste de carga

```powershell
python tools/load_test.py
```

## Limitações conhecidas

- Rate limiter usa janela fixa (não sliding window) e não expira IPs inativos da memória.
- Uma nova conexão TCP é aberta com o backend a cada requisição (sem connection pooling).
- Parser HTTP do endpoint de métricas é minimalista, não um servidor HTTP genérico.
- API key do dashboard está hardcoded no código-fonte (adequado para demonstração; produção exigiria gestão de segredos).
- JSON de métricas é montado manualmente (sem escaping), adequado ao escopo controlado atual.

## Melhorias futuras

- Connection pooling com os backends
- Sliding window ou token bucket para rate limiting
- Suporte a HTTPS/TLS
- Build incremental via CMake

## Autor

Marienny Azevedo
