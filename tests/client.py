import string
import random
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from timeit import default_timer as timer

SERVER_ADDRESS = "localhost"
SERVER_PORT = 8080
RESOURCE = "teruel.mp4"
EXTENSION = RESOURCE[RESOURCE.find('.'):]


def get_data(messages):
    for _ in range(messages):
        filename = ''.join(random.choice(string.ascii_letters) for _ in range(30))
        urllib.request.urlretrieve(f"http://{SERVER_ADDRESS}:{SERVER_PORT}/{RESOURCE}",
                                   f"received/{filename}.{EXTENSION}")


def start_stress_test(num_threads=8, messages_per_thread=1):
    start = timer()

    with ThreadPoolExecutor(max_workers=num_threads) as pool:
        for _ in range(num_threads):
            pool.submit(get_data, messages_per_thread)

    end = timer()
    elapsed = end - start
    num_messages = num_threads * messages_per_thread
    speed = num_messages / elapsed

    print(f"[+] Took {elapsed} s to parse {num_threads} messages: average speed of {speed} messages/s")


start_stress_test(num_threads=8, messages_per_thread=10)
