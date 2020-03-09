# Rask Web Server
## Introducción
En esta primera práctica se ha implementado de un servidor web en C,
basado en el protocolo HTTP/1.1 (conexiones persistentes, sin pipelining).
Procedemos a describir los rasgos más importantes de este proyecto, desde las
decisiones de diseño hasta la estructura del código.
## Decisiones de diseño
Son varias las decisiones que se han tenido que tomar a lo largo de todo el desarrollo.

- La primera gran decisión a la que tuvimos que enfrentarnos fue el tipo de servidor a desarrollar.
Este paso puede considerarse como uno de los más importantes del proyecto, ya que determina, sin lugar a dudas, 
el rendimiento final del servidor. Esta decisión puede estructurarse en los siguientes pasos:
    1. **Procesos o hilos**: estaba claro desde el principio que el servidor debía contar con cierto grado de paralelismo,
pues los servidores iterativos son totalmente ineficientes en la gestión de peticiones simultáneas y no explotan al
máximo los recursos de las máquinas donde se ejecutan. Sin embargo, debíamos decidir si usar una mezcla de procesos e hilos
o simplemente usar hilos. La motivación de la primera opción es que en ciertos sistemas, la librería de hilos la controla
cada proceso, es decir, el procesador no ve hilos, solo ve procesos, por lo que el tiempo de ejecución se asigna a procesos,
y son estos los que distribuyen el tiempo entre sus hilos. Tras consultar diversas fuentes, la librería estándar POSIX de threads
hace que el procesador vea a esos hilos y por tanto consten de tiempo de ejecución propio. Como además los hilos son muchos más ligeros
y presentan menos overhead que los procesos, decidimos basar la concurrencia en threads. TODO: comentar mejor lo del estándar si hace falta

    2. **Thread por cliente o thread-pool**: el siguiente paso esa determinar qué tipo exacto de servidor íbamos a implementar,
una vez que teníamos claro el uso de threads. Las dos alternativas eran crear un thread por cliente o desarrollar un pool de threads.
La primera posibilidad tenía algunas ventajas, entre las que destacan la simpleza de implementación y la rapidez inicial de la respuesta.
Esto último se refiere a que, nada más llegue una petición desde un cliente, se le asigna un hilo, con el único retardo que genera la creación del thread.
Con respecto al pool, la respuesta a una petición puede ser inmediata si tenemos un hilo disponible, o puede retrarse si
no es así. Sin embargo, el pool de hilos ofrece un mayor control sobre la ejecución en todo momento, pudiendo gestionar dinámicamente
y de una forma mucho más directa el número de hilos disponibles.