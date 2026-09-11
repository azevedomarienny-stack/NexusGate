import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 8080
NUM_THREADS = 5
REQUESTS_PER_THREAD = 10

results = []
lock = threading.Lock()

def worker():
    for _ in range(REQUESTS_PER_THREAD):
        start = time.time()
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(5)
            s.connect((HOST, PORT))
            s.send(b"load-tests\n")
            s.recv(1024)
            s.close()
            elapsed = time.time() - start
            with lock:
                results.append(elapsed)
        except Exception:
            with lock:
                results.append(None)
                
threads = [threading.Thread(target=worker) for _ in range(NUM_THREADS)]


start_time = time.time()
for t in threads:
    t.start()
for t in threads:
    t.join()
total_time = time.time() - start_time

successes = [r for r in results if r is not None]
failures = len(results) - len(successes)

print(f"Total de requisicoes: {len(results)}")
print(f"Sucesso: {len(successes)}  Falhas/rejeitadas: {failures}")
print(f"Tempo total: {total_time:.2f}s")
if successes:
    print(f"Latencia media: {sum(successes)/len(successes)*1000:.2f} ms")
    print(f"Latencia maxima: {max(successes)*1000:.2f} ms")
print(f"Requisicoes/segundo: {len(results)/total_time:.1f}")