import sys
import signal
from urllib.parse import parse_qs
TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

params = parse_qs(sys.stdin.read())

if "temperature" in params:
    for temperature in params["temperature"]:
        try:
            print(f"{temperature} Celsius is {(float(temperature) * 9/5) + 32} Fahrenheit")
        except (ValueError):
            print(f"{temperature} is not a valid number!")

else:
    print("ERROR IN PARAMS. NO TEMPERATURE SPECIFIED\n")

print("\r")
