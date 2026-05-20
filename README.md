practica 3 - sistemas operativos
universidad nacional de colombia

--- archivos ---
p2-server.c   servidor principal
p2-client.c   cliente interactivo
hash.c        tabla hash (guarda posiciones en disco, no datos)
hash.h        definiciones de la tabla hash
protocolo.h   estructura de mensajes entre cliente y servidor
Makefile      compilacion
flights.csv   dataset (debe estar en la misma carpeta del servidor)

--- compilar ---
make

--- ejecutar ---
terminal 1: ./p2-server
terminal 2: ./p2-client

--- como funciona ---
el servidor lee flights.csv una sola vez al inicio y guarda en la
tabla hash solo la posicion (offset) de cada registro, no los datos.
cuando un cliente busca, el servidor va al disco con fseek() al
offset guardado y lee solo ese registro. asi la memoria no supera 1MB.

cada cliente es atendido por un proceso hijo (fork()). el padre
sigue esperando nuevos clientes. maximo 32 clientes simultaneos.

cada busqueda queda registrada en server.log con el formato:
[20260510T143022] Cliente [127.0.0.1] [busqueda - Recife - Florianopolis]

--- formato de comandos ---
el cliente envia un struct Peticion con:
  opcion:  1=origen 2=destino 3=hora 4=buscar 5=salir
  origen:  ciudad de origen (busqueda parcial, sin importar mayusculas)
  destino: ciudad de destino (puede estar vacio)
  hora:    tipo de vuelo: firstClass / economic / premium (puede estar vacio)

el servidor responde un struct Respuesta con:
  encontrado:  1 si hay resultados, 0 si NA
  tiempoMedio: promedio de horas de vuelo
  cantVuelos:  cantidad de vuelos que coincidieron
  mensaje:     texto con el resultado

--- ejemplo de uso ---
1. ejecutar ./p2-server en una terminal
2. ejecutar ./p2-client en otra terminal
3. opcion 1 -> ingresar: Recife
4. opcion 2 -> ingresar: Florianopolis
5. opcion 4 -> ver resultado: tiempo medio 1.76 horas (7609 vuelos)

--- dataset flights.csv ---
campos: travelCode, userCode, from, to, flightType, price, time, distance, agency, date
busqueda por: from (origen), to (destino), flightType (tipo)
resultado: promedio del campo time (horas de vuelo)
