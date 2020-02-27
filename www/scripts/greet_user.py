import sys
import signal
from urllib.parse import parse_qs
TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

params = parse_qs(sys.stdin.read())

if "name" in params:
    for name in params["name"]:
        print(f"Hey {name}! Nice to meet you.")
else:
    print("ERROR IN PARAMS. NO NAME SPECIFIED")

print("\r")
