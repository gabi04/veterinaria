#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define PORT 3533
#define BACKLOG 32
#define MAX_PROCESOS 1 //numero maximo de procesos que deja entrar el semaforo al tiempo, NO modificar.
#define MAX_buff 256

/* Variable para semaforo*/
pthread_mutex_t mutex_dataDogs, mutex_log;
int num_registro;
int serverfd;

void stop(){
	close(serverfd);
}

/*________________________________Estructura dogType_____________________________________*/
struct dogType{
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;
	int prev;
	int next;
};

/*________________________________Estructura perroHash___________________________________*/
struct perroHash{
	int primerPerro;
	int ultimoPerro;
};

/*________________________________Estructura threadData___________________________________*/
struct threadData{
	int clientfd;
	char IP[INET_ADDRSTRLEN];
};

/*________________________________Imprimir estructura dogType____________________________*/
void imprimir(struct dogType *perro){

	printf("Nombre: %s \n", perro->nombre);
	printf("Tipo: %s \n", perro->tipo);
	printf("Edad: %i \n", perro->edad);
	printf("Raza: %s \n", perro->raza);
	printf("Estatura: %i \n", perro->estatura);
	printf("Peso: %f \n", perro->peso);
	printf("Sexo: %c \n", perro->sexo);
	printf("Prev: %i \n", perro->prev);
	printf("Next: %i \n", perro->next);

}

/*______________________Calcula el codigo hash de una cadena de caracteres_______________*/
unsigned int funcion_hash(char *s) {

	unsigned int h;
	unsigned const char *us;

	int i;
	for(i = 0; i < strlen(s); i++){	s[i] = tolower(s[i]); }
	us = (unsigned const char *) s;
	h = 0;
	while(*us != '\0') { h = (h * 256 + *us) % 1187; us++; }
	return h;

}

/*______________________Inicializa la variable num_registro______________________________*/
void initNumRegistro(){

    FILE *archivo;

    archivo = fopen("dataDogs.dat", "a+");
    if(archivo == NULL){
        perror("Error al abrir el archivo dataDogs. \n");
        exit(-1);
    }

    fseek(archivo, 0, SEEK_END);
    num_registro = ftell(archivo) / sizeof(struct dogType) + 1;

    fclose(archivo);

}

/*______________________Borrar la ultima estructura del archivo__________________________*/
void sobreescribir(){


    FILE *origen;
    int new_size;

    //Las dos funciones siguientes recortan un archivo a la talla off_t length
    //int truncate(const char *path, off_t length);
    //int ftruncate(int fd, off_t length);

    //abre archivo origen
    origen= fopen("dataDogs.dat", "r");
    if(origen == NULL){ perror("error al crear archivo"); exit(-1); }
    fseek(origen,0L,SEEK_END);
    new_size = ftell(origen)-sizeof(struct dogType);
    fclose(origen);

    truncate("dataDogs.dat", new_size);

    num_registro--;
}

/*_____________________________Ingresar nueva estructura dogType___________________________*/
void ingresar(struct dogType *perro){

	int mut, hash, error, pos, pos_temp;
	FILE *dataDogs;
	FILE *tablaHash;
	struct perroHash *pos_hash;
	pos_hash = malloc(sizeof(struct perroHash));
	struct dogType *perro_temp;
	perro_temp = malloc(sizeof(struct dogType));
 
	//bloquear mutex
	mut = pthread_mutex_lock(&mutex_dataDogs);
	if(mut != 0){perror("error mutex_dataDogs lock"); exit(-1);}

	//abrir archivos	
	dataDogs = fopen("dataDogs.dat", "r+");
	if(dataDogs == NULL){ perror("Error abrir archivo dataDogs: r+"); exit(-1); }
	tablaHash = fopen("tablaHash.dat", "r+");
	if(tablaHash == NULL){ perror("Error abrir archivo tablaHash: r+"); exit(-1); }

	//Calcula la posicion en que se va a guardar el perro nuevo.
	fseek(dataDogs, 0, SEEK_END);
	pos = ftell(dataDogs) + 1;
	//printf("posicion: %ld\n", pos);

	//Obtiene un codigo hash correspondiente al nombre del perro nuevo.
	hash = funcion_hash(perro->nombre);
	//printf("hash %i\n", hash);

	//Busca la estructura hash asociada al perro nuevo en tablaHash
	fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
	error = fread(pos_hash, sizeof(struct perroHash), 1, tablaHash);
	if(error<=0){ perror("Error fread tablaHash"); exit(-1); }

	//Inserta la posicion del perro en la hashtable si es el primero con este codigo (1)
	//Si hay una colicion lo inserta al final de una linked list(2)
	/*(1)*/if(pos_hash->primerPerro == 0){
	        pos_hash->primerPerro = pos;
	}/*(2)*/else{

		//Encuentra el ultimo perro con el hash
		pos_temp = pos_hash->ultimoPerro - 1;
		fseek(dataDogs, pos_temp, SEEK_SET);
		error = fread(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
		if(error<=0){ perror("Error fread perror_temp en dataDogs"); exit(-1); }

		//Actualiza los datos de perro_temp y perro
		perro_temp->next = pos;
		perro->prev = pos_hash->ultimoPerro;

		//Escribe el ultimo perro en datosDogs.dat de nuevo
		fseek(dataDogs, pos_temp, SEEK_SET);
		error = fwrite(perro_temp , sizeof(struct dogType) , 1 , dataDogs);
		if(error == 0){ perror("Error fwrie perro_temp en dataDogs"); exit(-1); }
	}

	//guarda el perro nuevo al final del archivo
	fseek(dataDogs, 0, SEEK_END);
	error = fwrite(perro , sizeof(struct dogType) , 1 , dataDogs);
	if(error == 0){ perror("error fwrite perro en dataDogs"); exit(-1);}

	pos_hash->ultimoPerro = pos;

	//Escribe la nueva pareja p_hash en tabla
	fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
	error = fwrite(pos_hash , sizeof(struct perroHash), 1 , tablaHash);
	if(error == 0){ perror("Error fwrite pos_hash en tablaHash"); exit(-1);}

	//Libera recursos
	fclose(dataDogs);
	fclose(tablaHash);
	free(perro_temp);
	free(pos_hash);

    	num_registro++;
	
	//Desbloquear mutex
	mut = pthread_mutex_unlock(&mutex_dataDogs);
	if(mut != 0){perror("error mutex_dataDogs unlock"); exit(-1);}


}

/*______________________Abrir historia clinica del perro numero v e imprimirlo_______________________*/
int ver(int v, struct dogType *perro){

	FILE *dataDogs;
	int flag, error, id, mut;
	
	/*__________________Verificar que el numero de registro sea vailido___________________*/
	if(v > 0 && v < num_registro){

		//Bloqueat mutex
		mut = pthread_mutex_lock(&mutex_dataDogs);
		if(mut != 0){perror("Error mutex_dataDogs lock"); exit(-1);}

		//Abre archivo.
		dataDogs = fopen("dataDogs.dat", "r+");
		if(dataDogs == NULL){ perror("Error abrir archivo dataDogs"); exit(-1); }

		//Leer perro
		fseek(dataDogs, sizeof(struct dogType)*(v-1), SEEK_SET);
		error = fread(perro, sizeof(struct dogType), 1, dataDogs);
		if(error <= 0){ perror("Error fread archivo dataDogs."); exit(-1); }
		fclose(dataDogs);
	
		//Desbloquear mutex
		mut = pthread_mutex_unlock(&mutex_dataDogs);
		if(mut != 0){perror("Error mutex_dataDogs unlock"); exit(-1);}

		flag = 1;

	}else{
		flag = 0;
	}

	return flag;

}

/*______________________Borrar el perro numero v_________________________________________*/
int borrar(int b){
	
	int mut, error, hash, flag;
	long pos;
	FILE *dataDogs;
	FILE *tablaHash;
	struct perroHash *pos_hash;
	pos_hash = malloc(sizeof(struct perroHash));
	struct dogType *perro;
	perro = malloc(sizeof(struct dogType));
	struct dogType *perro_temp;
	perro_temp = malloc(sizeof(struct dogType));
	
	//Verificar que el registro es valido.
    	if(b > 0 && b < num_registro){
		
		flag = 1;
		
		//Bloquear mutex
		mut = pthread_mutex_lock(&mutex_dataDogs);
		if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}

		//Abrir archivos
		dataDogs = fopen("dataDogs.dat", "r+");
		if(dataDogs == NULL){ perror("Error fopen dataDogs: r+"); exit(-1); }
		tablaHash = fopen("tablaHash.dat", "r+");
		if(tablaHash == NULL){ perror("Error fopen tablaHash: r+"); exit(-1); }

		//Calcula la posicion del perro a borrar
		pos = (b-1)*sizeof(struct dogType) + 1;

		//Lee el perro a borrar del archivo
		fseek(dataDogs, pos-1, SEEK_SET);
		error = fread(perro, sizeof(struct dogType) , 1 , dataDogs);
		if(error == 0){ perror("Error fread perro"); exit(-1); }
		
		//Rescata la función hash asociada al nombre del perro a borrar
		hash = funcion_hash(perro->nombre);

		//busca la estructura perroHash h en tablaHash
		fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
		error = fread(pos_hash, sizeof(struct perroHash), 1, tablaHash);
		if(error == 0){ perror("Error fread tablaHash"); exit(-1); }

		//Leer el perro previo y el siguiente al perro a borrar y editarlos
		if(perro->prev == 0)
			pos_hash->primerPerro = perro->next;
		else{
			fseek(dataDogs, perro->prev-1, SEEK_SET);
			error = fread(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
			if(error == 0){ perror("Error leer uno"); exit(-1); }
			perro_temp->next = perro->next;
			fseek(dataDogs, perro->prev-1, SEEK_SET);
			error = fwrite(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
			if(error == 0){ perror("Error escribir uno"); exit(-1); }
		}
		if(perro->next == 0)
			pos_hash->ultimoPerro = perro->prev;
		else{
			fseek(dataDogs, perro->next-1, SEEK_SET);
			error = fread(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
			if(error == 0){ perror("Error fread dos"); exit(-1); }
			perro_temp->prev = perro->prev;
			fseek(dataDogs, perro->next-1, SEEK_SET);
			error = fwrite(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
			if(error == 0){ perror("Error fwrite dos"); exit(-1); }
		}
		
		
		//Escribe la nueva pareja p_hash en la tabla
		fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
		error = fwrite(pos_hash , sizeof(struct perroHash) , 1 , tablaHash);
		if(error == 0){ perror("Error fwrite tablaHash"); exit(-1); }
		
		//Calcula la posicion del ultimo perro de todo el archivo
		fseek(dataDogs, -sizeof(struct dogType), SEEK_END);

		if(ftell(dataDogs) != pos-1){
			error = fread(perro, sizeof(struct dogType) , 1 , dataDogs);
			if(error == 0){ perror("Error fread ultimo perro"); exit(-1); }

			//Rescata la función hash asociada al nombre del ultimo perro
			hash = funcion_hash(perro->nombre);

			//busca la estructura perroHash hash en tablaHash
			fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
			error = fread(pos_hash, sizeof(struct perroHash), 1, tablaHash);
			if(error == 0){ perror("Error fread pos_hash"); exit(-1); }

			//Lee el perro previo y el siguiente al ultimo perro de la lista y los edita
			if(perro->prev == 0)
				pos_hash->primerPerro = pos;
			else{
				fseek(dataDogs, perro->prev-1, SEEK_SET);
				error = fread(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
				if(error == 0){ perror("Error leer tres"); exit(-1); }
				perro_temp->next = pos;
				fseek(dataDogs, perro->prev-1, SEEK_SET);
				error = fwrite(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
				if(error == 0){ perror("Error escribir tres"); exit(-1); }
			}
			if(perro->next == 0)
				pos_hash->ultimoPerro = pos;
			else{
				fseek(dataDogs, perro->next-1, SEEK_SET);
				error = fread(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
				if(error == 0){ perror("Error fread dos"); exit(-1); }
				perro_temp->prev = pos;
				fseek(dataDogs, perro->next-1, SEEK_SET);
				error = fwrite(perro_temp, sizeof(struct dogType) , 1 , dataDogs);
				if(error == 0){ perror("Error fwrite dos"); exit(-1); }
			}
			//Escribe la nueva pareja p_hash en tabla
			fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
			error = fwrite(pos_hash, sizeof(struct perroHash) , 1 , tablaHash);
			if(error == 0){ perror("Error fwrite en tablaHash"); exit(-1); }

			//escribe el ultimo perro del archivo en la posicion del perro a borrar
			fseek(dataDogs, pos-1, SEEK_SET);
        		error = fwrite(perro, sizeof(struct dogType) , 1 , dataDogs);
        		if(error == 0){ perror("Error fwrite dataDogs"); exit(-1); }
		}
		
		//liberar recursos
		free(perro); 
		free(perro_temp);
		free(pos_hash);
		fclose(dataDogs);
		fclose(tablaHash);
		sobreescribir();

		//Desbloquear mutex
		mut = pthread_mutex_unlock(&mutex_dataDogs);
		if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}

	}
	else{
		flag = 0;
	}

	return flag;
}

int buscar(char nombre[32], int *positions, int *cont){

	FILE *dataDogs;
	FILE *tablaHash;
	struct dogType *perro;
	perro = malloc(sizeof(struct dogType));
	struct perroHash *pos_hash;
	pos_hash = malloc(sizeof(struct perroHash));
	int error, hash, flag, i, sonIguales, mut;
	long pos;

	*cont = 0;

	//Bloquear mutex
	mut = pthread_mutex_lock(&mutex_dataDogs);
	if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}
	
	//Abrir archivos	
	dataDogs = fopen("dataDogs.dat", "r");
	if(dataDogs == NULL){ perror("error al abrir archivo dataDogs:  r"); exit(-1); }
	tablaHash = fopen("tablaHash.dat", "r");
	if(tablaHash == NULL){ perror("error al abrir archivo tablaHash: r"); exit(-1); }

	//recupera el hash para ese nombre
	hash = funcion_hash(nombre);

	//busca la estructura perroHash hash en tablaHash
	fseek(tablaHash, hash*sizeof(struct perroHash), SEEK_SET);
	error = fread(pos_hash, sizeof(struct perroHash), 1, tablaHash);
	if(error == 0){ perror("Error fread tablaHash"); exit(-1); }

	//Si no existe una posicion asociada al codigo hash termina el programa(1)
	//De lo contrario indica la posicion en el archivo de la primer estructura con ese codigo hash(2)
	//printf("Tabla_hash[%i]: %ld\n", h, tabla_hash[h]);
	if(pos_hash->primerPerro == 0){
		flag = 0;
	}else{
		flag = 1;
		pos = pos_hash->primerPerro-1;
		
		do{
            		fseek(dataDogs, pos, SEEK_SET);
            		error = fread(perro, sizeof(struct dogType) , 1 , dataDogs);
            		if(error == 0){ perror("Error fread"); exit(-1); }
            		
			//funcion para comparar perro->nombre con nombre
			sonIguales = 1;
	    		for(i = 0; i < strlen(nombre); i++){
				if(perro->nombre[i] != nombre[i]){
					sonIguales = 0;
					break;
				}
	    		}

	    		if(sonIguales){
				positions[*cont] = pos;
				*cont+=1;
	    		}
			
			pos = perro->next - 1;			
	    
        	}while(perro->next != 0);

		if(*cont == 0)
			flag = 0;
	}

	//libera recursos
	free(perro); 
	free(pos_hash);
	fclose(dataDogs); 
	fclose(tablaHash);

	//Desbloquear Mutex
	mut = pthread_mutex_unlock(&mutex_dataDogs);
	if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}

	return flag;

}

int registro_serverDogs(char IP[INET_ADDRSTRLEN], int opcion, int reg, char cadena[32]){
	FILE *log;
	time_t tiempo;
	struct tm *tlocal;
	char output[14], regCompleto[50], regC[10];
	int m, r;
	
	
	m = pthread_mutex_lock(&mutex_log);
	if(m != 0){perror("error mutex_log"); exit(-1);}

	/*_________________________________Abrir archivo serverDogs.log________________________*/
	log = fopen("serverDogs.log", "a+");
	if(log == NULL){perror("error abrir serverDogs"); exit(-1); }
	
	//Calcular la hora y fecha actual
	tiempo = time(0);
	tlocal = localtime(&tiempo);
	strftime(output, 14, "%y%m%d%H%M%S", tlocal);
	
	strcpy(regCompleto, "[");
	
	switch(opcion){
			case 1: //insertar
	
				strcat(regCompleto, output); strcat(regCompleto, "]");
				strcat(regCompleto, "["); strcat(regCompleto, IP); strcat(regCompleto, "]");
				strcat(regCompleto, "[insertar][nuevoPerro]");
				break;
				
			case 2: //ver

				sprintf(regC, "%i", reg);
				
				strcat(regCompleto, output); strcat(regCompleto, "]");
				strcat(regCompleto, "["); strcat(regCompleto, IP); strcat(regCompleto, "]");
				strcat(regCompleto, "[ver]");
				strcat(regCompleto, "["); strcat(regCompleto, regC); strcat(regCompleto, "]");
				break;
				
			case 3: //borrar
				sprintf(regC, "%i", reg);
				
				strcat(regCompleto, output); strcat(regCompleto, "]");
				strcat(regCompleto, "["); strcat(regCompleto, IP); strcat(regCompleto, "]");
				strcat(regCompleto, "[borrar]");
				strcat(regCompleto, "["); strcat(regCompleto, regC); strcat(regCompleto, "]");
				break;
				
			case 4: //buscar
				
				strcat(regCompleto, output); strcat(regCompleto, "]");
				strcat(regCompleto, "["); strcat(regCompleto, IP); strcat(regCompleto, "]");
				strcat(regCompleto, "[buscar]");
				strcat(regCompleto, "["); strcat(regCompleto, cadena); strcat(regCompleto, "]");
				break;
			printf("________________________________________________________________________________\n");
	}
	
	//printf("regCompleto: %s, size: %i\n", regCompleto, sizeof(regCompleto));
	
	r = fwrite(regCompleto, sizeof(regCompleto), 1, log);
	if(r <= 0) {perror("Error write en serverDogs"); exit(-1);}
	
	fclose(log);

	m = pthread_mutex_unlock(&mutex_log);
	if(m != 0){perror("error mutex_log"); exit(-1);}
	
}

/*________________________________Imprimir archivo dataDogs.dat__________________________*/
void mostrarTodos(){

    FILE *dataDogs;
    struct dogType *perro;
    perro = malloc(sizeof(struct dogType));

    dataDogs = fopen("dataDogs.dat", "r+");
    if(dataDogs == NULL){ perror("error al crear archivo dataDogs"); exit(-1); }

    while(fread(perro, sizeof(struct dogType), 1, dataDogs) != 0){
	printf("%ld \n", ftell(dataDogs)-sizeof(struct dogType));
        imprimir(perro);
    }

    free(perro);
    fclose(dataDogs);

}

int *nuevaConexion(void *thrData){
	
	struct threadData *tD; 
	tD = (struct threadData*)thrData;
	int error, opcion, v, b, flag, i, mut, clientfd = tD->clientfd, size, cont;
	int positions[1000];
    	char nombre[32];
	struct dogType *perro;
	FILE *histClinica;
	ssize_t rcvd_bytes;
	
	/*_________________________________Recibir mensajes del cliente________________________*/
	do{
		error = recv(clientfd, &opcion, sizeof(opcion), 0);
		if(error != sizeof(opcion)){perror("Error recv1"); exit(-1);}

		switch(opcion){
			case 1:				
				perro = malloc(sizeof(struct dogType));
				error = recv(clientfd, perro, sizeof(struct dogType), 0);
				if(error == -1){ perror("error recv 2"); exit(-1); }	
				ingresar(perro);			

				registro_serverDogs(tD->IP, opcion, opcion, (char) opcion);
				free(perro);
			break;

			case 2:
				perro = malloc(sizeof(struct dogType));

				error = send(clientfd, &num_registro, sizeof(num_registro), 0);
				if(error == -1){ perror("error send 1"); exit(-1); }

				error = recv(clientfd, &v, sizeof(v), 0);
				if(error == -1){ perror("error recv 3"); exit(-1); }
				
				flag = ver(v, perro);
				
				error = send(clientfd, &flag, sizeof(int), 0);
				if(error == -1){ perror("error send 2"); exit(-1); }
								
				if(flag == 1){
					
					error = send(clientfd, perro, sizeof(struct dogType), 0);
					if(error == -1){ perror("error send 3"); exit(-1); }
									   
					//Crea/abre archivo con historia clinica
					char nombreArch[40];
					strcpy(nombreArch, perro->nombre);
					strcat(nombreArch, ".txt");
					//printf("%s", nombreArch);
					histClinica = fopen(nombreArch, "a+");
					if(histClinica == NULL){ perror("Error fopen archivo histC"); exit(-1); }
 
					//Calcula el tamaño del archivo
					fseek(histClinica, 0L, SEEK_END);
					size = ftell(histClinica);
					
					if(size == 0){
					    error = fprintf(histClinica, "%s %s", "Historia clínica de", perro->nombre);
					    if(error < 0){ perror("error fprintf"); exit(-1); }

					    //Calcula el tamaño del archivo de nuevo
					    fseek(histClinica, 0L, SEEK_END);
					    size = ftell(histClinica);
					}
					
					
					//Envía el tamaño del archivo
					error = send(clientfd, &size, sizeof(int), 0);
					if(error == -1){ perror("error send size"); exit(-1); }				

					fclose(histClinica);
					
					histClinica = open(nombreArch, O_RDONLY);
					if(histClinica == NULL){ perror("error al crear archivo histC"); exit(-1); }

					ssize_t read_bytes, /* bytes read from local file */
							sent_bytes; /* bytes sent to connected socket */
							
	
					char buff[MAX_buff]; /* max chunk size for sending file */
					//int newSendH /* file handle for reading local file*/
					
					do{
					   read_bytes = read(histClinica, &buff, MAX_buff);
					   if(read_bytes == -1){ perror("error read_bytes"); exit(-1); }
					   
					   //printf("read_bytes: %i\n", read_bytes);
					   
					   sent_bytes = send(clientfd, buff, read_bytes, 0);
					   if(sent_bytes == -1){ perror("error send chunk"); exit(-1); }
					   //printf("sent_bytes: %i\n", sent_bytes);
					
					   size -= sent_bytes;
					}while(sent_bytes < size);
	
					//---------------------------------------------
                    
					//Cierra archivo.
					close(histClinica);

					//Recibir archivo de vuelta---------------------

					error = recv(clientfd, &size, sizeof(int), 0);
					if(error == -1){ perror("error recv size"); exit(-1); }
					
					histClinica = open(nombreArch, O_WRONLY);
					if(histClinica == NULL){ perror("error al crear archivo histC"); exit(-1); }
					
					do{
					   rcvd_bytes = recv(clientfd, buff, MAX_buff, 0);
					   if(rcvd_bytes == -1){ perror("error send 4"); exit(-1); }
					   //printf("rcvd_bytes: %i\n", rcvd_bytes);
					   
					   error = write(histClinica, buff, rcvd_bytes);
					   if(error == -1){ perror("error write"); exit(-1); }
					   //printf("write: %i\n", r);

					   size -= rcvd_bytes;
					}while(rcvd_bytes < size);
					
					close(histClinica);
					//------------------------------------------------------------------------

				}

				registro_serverDogs(tD->IP, opcion, v, "op2");
				free(perro);
			break;

			case 3:
				error = send(clientfd, &num_registro, sizeof(num_registro), 0);
				if(error == -1){ perror("error send 4"); exit(-1); }

				error = recv(clientfd, &b, sizeof(b), 0);
				if(error == -1){ perror("error recv 4"); exit(-1); }
				
				flag = borrar(b);
			
				error = send(clientfd, &flag, sizeof(flag), 0);
				if(error == -1){ perror("error send 5"); exit(-1); }
			
				registro_serverDogs(tD->IP, opcion, b, (char) b);					
			break;

			case 4:
				error = recv(clientfd, nombre, sizeof(nombre), 0);
				if(error == -1){ perror("error recv 5"); exit(-1); }
				
				flag = buscar(nombre, positions, &cont);

				error = send(clientfd, &flag, sizeof(flag), 0);
				if(error == -1){ perror("error send 6"); exit(-1); }
				
				
				if(flag == 1){
					perro = malloc(sizeof(struct dogType));
					error = send(clientfd, &cont, sizeof(cont), 0);
					if(error == -1){ perror("error send 7"); exit(-1); }	

					FILE * dataDogs;

					mut = pthread_mutex_lock(&mutex_dataDogs);
					if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}

					dataDogs = fopen("dataDogs.dat", "r");
					if(dataDogs == NULL){ perror("error al abrir archivo dataDogs r: "); exit(-1); }
					for(i = 0; i < cont; i++){
						fseek(dataDogs, positions[i], SEEK_SET);
		    				error = fread(perro, sizeof(struct dogType) , 1 , dataDogs);
		    				if(error == 0){ perror("no se puede leer"); exit(-1); }
						error = send(clientfd, perro, sizeof(struct dogType), 0);
						if(error == -1){ perror("error send 8"); exit(-1); }
						//imprimir(perro);
							
					}

					free(perro);
					fclose(dataDogs);

					mut = pthread_mutex_unlock(&mutex_dataDogs);
					if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}	
				}
				registro_serverDogs(tD->IP, opcion, opcion, nombre);
			break;

			case 5:
			break;

			case 6:
				mostrarTodos();
			break;

			default:
				printf("Opcion no valida, intenta de nuevo.\n");
			break;
			printf("________________________________________________________________________________\n");

		}	

	}while(opcion!=5);
	
}

int main(){
   
	int clientfd, error, opcion, mut;
	struct sockaddr_in server, client;
	socklen_t size = sizeof(struct sockaddr_in);
	socklen_t sizecli;	
	
	initNumRegistro();
        
	
	/*_________________________________Configurar servidor_________________________________*/	
	//Crear un socket
	serverfd = socket(AF_INET, SOCK_STREAM,0);
	if(serverfd == -1){ perror("error socket servidor"); exit(-1); }
	//Configurar el socket
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr= INADDR_ANY;
	bzero(server.sin_zero, 8);
	error = bind(serverfd, (struct sockaddr *)&server, size);
	if(error == -1){ perror("error bind"); exit(-1); }
	//Aclaramor que la comunicaciones son entrantes
	error = listen(serverfd, BACKLOG);
	if(error == -1){ perror("error listen"); exit(-1); }
	
	//Crear mutex para gestionar zonas criticas
    	mut = pthread_mutex_init(&mutex_dataDogs, NULL);
	if(mut != 0){perror("error mutex_dataDogs"); exit(-1);}
    	mut = pthread_mutex_init(&mutex_log, NULL);
	if(mut != 0){perror("error mutex_log"); exit(-1);}		

	//Aceptar comunicaciones con el socket
	while(clientfd = accept(serverfd, (struct sockaddr *)&client, (socklen_t *)&sizecli)){

		if(signal(SIGINT, stop) == SIG_ERR){
			perror("Signal failed");
		}
        	if(clientfd == -1){ perror("error accept"); exit(-1); }

        	pthread_t nuevoHilo;
		struct threadData tD;
		tD.clientfd = clientfd;
		inet_ntop(AF_INET, &(client.sin_addr), tD.IP, INET_ADDRSTRLEN);

		//Crear hilo
		error = pthread_create(&nuevoHilo, NULL, (void *)nuevaConexion, (void *)&tD);
		if (error != 0){
			perror ("No puedo crear hilo");
			exit (-1);
        	}
	}

	

	/*_______________________________________Finalizar_____________________________________*/   
	//liberar recursos
	pthread_mutex_destroy(&mutex_dataDogs);
	pthread_mutex_destroy(&mutex_log);
	close(clientfd);
	close(serverfd);
	printf("fin del servidor \n");  
}
