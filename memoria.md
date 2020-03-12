# Rask Web Server
## Introducción
En esta primera práctica se ha implementado de un servidor web en C,
basado en el protocolo HTTP/1.1 (conexiones persistentes, sin pipelining).
Procedemos a describir los rasgos más importantes de este proyecto, desde las
decisiones de diseño hasta la estructura del código.

## Decisiones de diseño
Son varias las decisiones que se han tenido que tomar a lo largo de todo el desarrollo.

### Paralelismo

La primera gran decisión a la que tuvimos que enfrentarnos fue el tipo de servidor a desarrollar.
Este paso puede considerarse como uno de los más importantes del proyecto, ya que determina, sin lugar a dudas, el rendimiento final del servidor. Esta decisión puede estructurarse en los siguientes pasos:

  1. **Procesos o hilos**: estaba claro desde el principio que el servidor debía contar con cierto grado de paralelismo, pues los servidores iterativos son totalmente ineficientes en la gestión de peticiones simultáneas y no explotan al máximo los recursos de las máquinas donde se ejecutan. Sin embargo, debíamos decidir si usar una mezcla de procesos e hilos o simplemente usar hilos. El principal argumento a favor de usar varios procesos es que en algunos sistemas, el *kernel* tan sólo es consciente de la existencia de los procesos y no de los hilos. Esto es lo que se conoce como hilos a nivel de usuario y tienen principalmente dos desventajas. La primera es que cuando un hilo hace una llamada bloqueante al sistema, como un *accept*, por ejemplo, el resto de hilos también se bloquean. La segunda es que una aplicación multihilo no podría aprovechar varios núcleos del procesador. Leyendo el manual de *pthreads* de Linux, vemos que la implementación (es importante recalcar que *pthreads* es una interfaz, y que cada sistema operativo lo implementa de una manera distinta) es 1:1, es decir, cada *thread* corresponde a una *kernel scheduling entity*. Esto es, el sistema operativo es consciente de la existencia de los *threads*. Algo similar ocurre en el caso de macOS, aunque la implementación de *pthreads* es distinta, ya que usa *Mach threads*. Es por esto que decidimos implementar el servidor usando *threads*, pues son ligeros, fáciles de utilizar, y cumplen con la funcionalidad necesaria.

  2. ***Thread* por cliente o *thread-pool***: el siguiente paso era determinar qué tipo exacto de servidor íbamos a implementar, una vez que teníamos claro el uso de *threads*. Las dos alternativas eran crear un *thread* por cliente o desarrollar un *pool* de threads. La primera posibilidad parecía a priori muy fácil de implementrar (aunque como siempre el diablo está en los detalles). Un pequeño inconveniente que tendría esto sería esperar a la generación de un hilo cada vez que el *accept* retornase. No obstante, el tiempo de generación de un hilo es prácticamente despreciable, del orden de décimas de milisegundo. Como hemos comentado antes, el problema está en los detalles. Por ejemplo, tenemos que almacenar los *pthread_t* de cada hilo que esté corriendo para poder pedirles que paren en el caso de recibir la señal de apagado y para hacer el *pthread_join* después. Para almacenar los *pthread_t*  de manera eficiente, deberíamos saber cuándo un hilo ha terminado para eliminar el *pthread_t* de la lista. En definitiva, a la hora de implementarlo surgirían ciertos problemas, algunos de los cuales comunes a ambas alternativas de diseño.  Es por eso que decidimos usar un *thread-pool*, ya que es lo que se nos recomendó en clase, no tanto por su rapidez, pues los hilos ya están pre-creados, sino por la facilidad de su gestión.

  3. ***Pool* estático o dinámico**: una vez que nos decantamos por el *pool* de hilos, nos planteamos si desarrollar un *poo*l estático, en el que se creara desde el inicio el número de hilos máximo, o bien implementar un *pool* dinámico, que se autogestionase en función del número de hilos en ejecución en cada momento. Si bien la primera opción es mucho más sencilla y garantiza que cada cliente va a ser atendido inmediatamente, supone un gasto innecesario de recursos. Un *thread* en Linux ocupa aproximadamente 16 KiB de memoria, lo cual es casi despreciable en los tiempos que corren. No obstante, para realizar un servidor escalable hemos optado por un *pool* dinámico, ya que en el caso de tener un número máximo de conexiones muy elevado (del orden de 10000) pero muy poca carga de trabajo se estarían desperdiciando los recursos del ordenador. A continuación se muestra el uso de memoria del servidor en *idle* en función del número de *threads* activos (bajo Ubuntu 18.04). 

     | Número de *threads* | Memoria usada |
     | ------------------- | ------------- |
     | 5                   | 564 KiB       |
     | 10                  | 660 KiB       |
     | 20                  | 828 KiB       |
     | 50                  | 1.3 MiB       |
     | 100                 | 2.1 MiB       |
     | 500                 | 8.6 MiB       |
     | 1000                | 16.6 MiB      |

     Como comparación,  `Apache` en su configuración por defecto y en *idle* también usa aproximadamente 8 MiB de memoria RAM.

 Pasando a la implementación concreta, la gestión la lleva a cabo un hilo aparte dentro del *pool* (el `watcher_thread`), que cada cierto tiempo comprueba cuántos hilos se encuentran ocupados, determinando si es necesario crear más o destruir algunos de los existentes. Este tiempo no debe ser muy grande, ya que necesitamos reaccionar rápido a sobrecargas repentinas. Además, no consume apenas recursos realizar esta comprobación. Cabe mencionar también que el *pool* tiene dos formas de destruirse, `soft` y `hard`, para poder cerrar de inmediato todas las conexiones o bien esperar a que los hilos acaben de atender las peticiones activas en ese momento para cerrar después. Esto lo asociamos a un *restart* o a un *stop* de un servicio, siendo el *restart* el modo `soft` (necesitamos que el servicio vuelva a levantarse, pero preferimos no cortar transmisiones), y el *stop* el modo `hard` (necesitamos que el servicio pare de inmediato).

 Por otro lado, hemos considerado que si los *threads* detectan un error operen igual que si se ha cerrado la conexión, evitando acceder a zonas de memoria no inicializadas pero no parando el flujo de ejecución. TODO: completar con razones más fuertes

### Instalación

Al ejecutar `sudo make install`, se copiarán los siguientes ficheros:

#### Ejecutable

Se copiará el ejecutable `./cmake-build-config/rask` a `/usr/local/bin`. Esto permitirá ejecutar el servidor llamando a `rask` desde cualquier terminal. Cabe mencionar que si se ejecuta directamente de esta manera no correrá en modo demonio. El motivo por el que hemos escogido `/usr/local/bin` y no `/usr/bin` es doble:

- En Linux `/usr/bin` está pensado para programas administrados por el gestor de paquetes (como `apt` en Ubuntu o `pacman` en Arch), mientras que `/usr/local/bin` es preferido para programas compilados localmente.
- En macOS no se puede modificar el directorio `/usr/bin` a no ser que se desactive SIP (*System Integrity Protection*).

#### Fichero de configuración

Se copiará el archivo de configuración `./files/rask.conf` a `/etc/rask/`, directorio donde es usual almacenar los archivos de configuración según el manual (ver `hier`).

#### Página web

Se copiará todo el contenido de `./www` a `/var/www`. Este es el directorio que usa por defecto Apache.

#### Archivo de unidad para systemd

Se copiará el fichero `./files/rask.service` a `/lib/systemd/system/` si la distribución de Linux soporta systemd.

### Demonio

Inicialmente implementamos el demonio según el libro de referencia (Unix Networking Programming), pero posteriormente acabamos por emplear `systemd` en Linux, por ser más sencillo, moderno y cómodo de utilizar. TODO: guillote, si logras hacer eso sin actualizar systemd explicalo aquí

### Librerías

En `srclib` se encuentran los archivos con funcionalidad independiente al servidor. En algunos de ellos, se han tomado decisiones importantes:

#### Execute scripts

Contiene el código encargado de ejecutar *scripts* en un proceso aparte, pasándole los argumentos por entrada estándar (stdin). Para ello, como es necesario escribir y leer del proceso que ejecuta el *script*, necesitamos hacer uso de *pipes* y no podemos usar `popen`, ya que 
este último solo permite la comunicación en un solo sentido. Además, hemos implementado un *timeout*. Aunque esto también pueda realizarse en los propios *scripts*, como desarrolladores de un servidor web genérico no podemos asumir que todos los usuarios de nuestro servidor vayan a implementar un *timeout* en sus scripts. 

#### Socket

Agrupa las funciones relacionadas con la gestión de los *sockets*. Comentamos brevemente las funciones más destacadas

- `socket_open`: llama a las rutinas `socket`, `bind` y `listen`.
- `socket_set_timeout`: fija un *timeout* para un socket concreto. Esto es usado en el servidor para establecer un límite de tiempo en el que nos bloqueamos en el *read* esperando la petición del cliente.
- `ip_to_string`: es una función interna que devuelve una cadena de caracteres con la representación usual de una dirección IP. Usamos esto para imprimir en un formato comprensible la dirección IP de los clientes del servidor.

#### String

Como la librería `picohttpparser` nos devuelve los campos de la cabecera con pares `char *` - `int` decidimos crear una estructura pública `string`, conteniendo tanto el puntero al inicio de la cadena de caracteres como el tamaño de esta. Esto simplifica el paso de argumentos. Además, se incluyen dos funciones, una para comprobar si dos estructuras strings son iguales y otra para comprobar si una estructura string es igual a una cadena de caracteres terminada en `\0`. 

#### Logging

Como en todo gran proyecto se ha de informar al usuario del estado del programa. Es por esto que desde el principio decidimos crear esta librería. Tiene una interfaz muy parecida a `printf` en el sentido que recibe primero una cadena de caracteres con el formato (usando "%s", "%d", etc.) seguida de un número variable de argumentos. El objetivo de esta librería es doble:

- Por una parte, se puede controlar el nivel de verbosidad del servidor. En la etapa de desarrollo, querremos ver todos los mensajes en el log. Sin embargo, en la fase de despliegue sólo querremos ver los mensajes de mayor severidad (advertencia y errores)
- Lograr que los *logs* sean iguales independientemente de si el servidor corre en modo *daemon* o en modo normal. Si los mensajes se ven a través de `systemd`, el sistema añadirá automáticamente la hora en la que fueron emitidos. Sin embargo, si se corre normalmente, esto no ocurriría de no ser por esta librería.

#### Dynamic Buffer

Esta librería es una abstracción de un *buffer* dinámico, esto es, un *buffer* al que se le añaden cosas, y que si se queda sin espacio hace un `realloc` para crecer. Internamente se mantiene una cadena de caracteres del tamaño que el cliente especifique y dos enteros para saber la última posición ocupada y el tamaño del *buffer*.  Su utilidad es doble:

- A la hora de leer la respuesta de un *script* no sabemos cómo de larga va a ser. No podríamos usar un *buffer* de tamaño fijo e ir mandándolo "a trozos" ya que debemos conocer el tamaño de la respuesta a la hora de escribir el `Content-Length`. Es por esto que un *buffer* dinámico es la solución perfecta.
- A la hora de crear una respuesta HTTP nos da mucha flexibilidad, ya que no tenemos que preocuparnos de que la cabecera no quepa en el *buffer*. Además, hemos incorporado numerosas funcionalidades: añadir una cadena de caracteres, un número, un fichero (entero), un fichero (por partes) y el contenido leído de un descriptor de fichero (últil para leer de un *pipe*). A la hora de mandar ficheros con el servidor se pueden utilizar las dos funciones. Nosotros hemos optado por mandar el fichero por partes, es decir, se leerá el fichero hasta el final del *buffer* (que por defecto tiene 4096 bytes, y, a no ser que la cabecera sea enorme, será el tamaño de este), se enviará, y se repetirá esta operación tantas veces como sea necesario. Más sencilla era nuestra implementación inicial, que era leer el archivo entero en el *buffer* y mandarlo directamente. La desventaja de esto era que si el fichero era muy grande el servidor consumía muchísima memoria. 

### Scripts

En relación con los scripts que ejecuta el servidor, realizamos los scripts propuestos en Python, que se encargan de gestionar los argumentos recibidos por entrada estándar en formato `url-encode`, y se pueden ejecutar tanto con el método `GET` como con el método `POST`. Añadimos al `index.html` un campo y un botón para poder ejecutarlos desde la web. Se considera además que se solicita la ejecución de un script a través de `GET`cuando el archivo solicitado tiene una extensión ejecutable (`.py` o `.php`), sin importar si se recibe argumentos o no. En cuanto a `POST`, consideramos que siempre se nos va a solicitar ejecutar un script (si no es de extensión ejecutable se responde con Bad Request TODO: este código hay que revisar si es el idóneo)

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

## Conclusiones

TODO
