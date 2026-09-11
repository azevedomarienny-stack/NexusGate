import socket, sys

port = int(sys.argv[1])
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(("127.0.0.1", port))
s.listen(5)
print(f"Backend fake rodando na porta {port}")

while True:
    conn, _ = s.accept()
    data = conn.recv(1024)
    if data: 
        conn.send(f"[backend:{port}] ".encode() + data)
    conn.close()