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
    y de una forma mucho más directa el número de hilos disponibles. Además, en varias de las fuentes consultadas se afirmaba que el rendimiento
    de esta implementación era superior al resto de opciones
    
    3. **Pool estático o dinámico**: una vez que nos decantamos por el pool de hilos, nos planteamos si desarrollar un pool estático, en el que
    se creara desde el inicio el número de hilos máximo, o bien implementar un pool dinámico, que se autogestionase en función del número de hilos en ejecución 
    en cada momento. Si bien la primera opción es mucho más sencilla y garantiza que cada cliente va a ser atendido inmediatamente, supone un gasto innecesario
    de recursos. Ilustrémoslo con un ejemplo: si nuestro servidor debe estar preparado para atender a 100 clientes, en el pool estático crearemos 100 hilos
    inicialmente. Ahora bien, si realmente no vamos a gestionar 100 conexiones continuamente, tenemos una gran parte de hilos que no aportan nada. Si esta gestión
    la realizamos de manera dinámica, podemos optimizar en gran medida el uso de recursos, creando hilos cuando se detecte que es necesario y matándolos cuando la
    carga de trabajo se encuentre por debajo de un umbral.
    
   Pasando a la implementación concreta, la gestión la lleva a cabo un hilo aparte dentro del pool (el `watcher_thread`), que cada cierto tiempo
   comprueba cuántos hilos se encuentran ocupados, determinando si es necesario crear más o destruir algunos de los existentes. Este tiempo no debe ser muy grande,
   ya que necesitamos reaccionar rápido a sobrecargas repentinas. Además, no consume apenas recursos realizar esta comprobación.
   
   Cabe mencionar también que el pool tiene dos formas de destruirse, `soft` y `hard`, para poder cerrar de inmediato o bien esperar
   a que los hilos acaben de atender las peticiones activas en ese momento para cerrar después. Esto lo asociamos a un restart o a un stop de un servicio,
   siendo el restart el modo `soft` (necesitamos que el servicio vuelva a levantarse, pero preferimos no cortar transmisiones), y
   el stop el modo `hard` (necesitamos que el servicio pare de inmediato).
   
   Por otro lado, hemos considerado que si los threads detectan un error operen igual que si se ha cerrado la conexión, evitando acceder a
   zonas de memoria no inicializadas pero no parando el flujo de ejecución. TODO: completar con razones más fuertes
   
- Demonio: TODO: comentar decisión final.

- Librerías: en srclib se encuentran los archivos con funcionalidad independiente al servidor. En algunos de ellos, se han tomado decisiones importantes:
    1. **execute_scripts**: este fichero contiene el código encargado de ejecutar scripts en un proceso aparte, pasándole los argumentos por entrada estándar (stdin).
    Para ello, como es necesario escribir y leer del proceso que ejecuta el script, necesitamos hacer uso de `pipes` y no podemos usar `popen`, ya que 
    este último solo permite la comunicación en un solo sentido. Valoramos la implementación de un timeout, haciendo uso de la función `select`, pero 
    tras consultar al profesor de teoría nos decantamos por no hacerlo. TODO: razones. No es difícil hacerlo como extra si queremos.
    2. **socket**: agrupa las funciones relacionadas con la gestión de los sockets. Cabe comentar la funcionalidad extra que maneja la función
    `socket_set_timeout`, fijando un timeout para un socket concreto. Esto es usado en el servidor para establecer un límite de tiempo en el que nos bloqueamos
    en el read esperando la petición del cliente.
    TODO: comentar decisión final de qué librerías se hacen. TODO: logging, dynamic_buffer, string...

## Organización y estructura de módulos

- A grandes rasgos, las posibles librerías desarrolladas se encuentran en el directorio srclib. Estos son ficheros independientes en gran 
  medida a las variantes de implementación del servidor, es decir, se ha intentado que fueran archivos que pudieran ser usados en cualquier otro proyecto,
  independientemente de las características del mismo. Cumplen su función sin depender del resto de módulos.
  
- Los ficheros que el servidor ofrece a sus clientes se encuentran en el directorio wwww, organizado en las carpetas media y scripts.

- El código fuente principal del servidor, encargado de la gestión, se encuentra en la carpeta src. Estos son los archivos dedicados a parsear 
el fichero de configuración del servidor, el archivo que gestiona el thread pool (en este directorio pues depende de la implementación del servidor), 
y los módulos encargados de procesar la petición (`request.c`), de construir la respuesta (`response.c`) y de entrelazar ambos (`connection_handler.c`).
Finalmente, el main del servidor se encuentra en `server.c`.

- Las cabeceras del código fuente se encuentran en el directorio includes. Además, aquí se encuentra el archivo `utils.h`, que contiene 
diversas macros con códigos de estado de peticiones o respuestas, necesarios en múltiples ficheros.

- Algunos tests desarrollados se encuentran en la carpeta tests, donde `client.py` que se ha implementado para realizar diversas pruebas de estrés a nuestro servidor
y `post.py` contiene una petición post con datos en el body.
