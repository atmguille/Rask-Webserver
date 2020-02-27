import sys
import signal
TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

celsius = int(sys.stdin.read())
fahrenheit = (celsius * 9/5) + 32

print(str(celsius) + " Celsius is " + str(fahrenheit) + " Fahrenheit\n\r\n")
