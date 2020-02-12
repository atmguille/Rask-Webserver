import socket
from concurrent.futures import ThreadPoolExecutor
from timeit import default_timer as timer

SERVER_ADDRESS = "127.0.0.1"
SERVER_PORT = 8080
MESSAGE = bytes("a" * 1024, "utf8")


def send_data(num_messages):
    for _ in range(num_messages):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((SERVER_ADDRESS, SERVER_PORT))
            sock.send(MESSAGE)
            response = sock.recv(4096)
            sock.close()


def start_stress_test(num_threads=8, num_messages=100):
    start = timer()

    with ThreadPoolExecutor(max_workers=num_threads) as pool:
        for _ in range(num_threads):
            pool.submit(send_data, num_messages//num_threads)

    end = timer()
    elapsed = end - start
    speed = num_messages / elapsed

    print(f"[+] Took {elapsed} s to parse {num_messages} messages: average speed of {speed} messages/s")


start_stress_test(num_threads=10, num_messages=100000)
