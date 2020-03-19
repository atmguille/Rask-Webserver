import sys
from urllib.parse import parse_qs

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
