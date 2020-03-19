import sys
from urllib.parse import parse_qs

params = parse_qs(sys.stdin.read())

if "name" in params:
    for name in params["name"]:
        print(f"Hey {name}! Nice to meet you.")
else:
    print("ERROR IN PARAMS. NO NAME SPECIFIED")

print("\r")
