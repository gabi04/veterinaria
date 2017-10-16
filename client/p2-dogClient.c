#include <sys/types.h> //socket, connect, send
#include <sys/socket.h> //socket, inet_addr, connect, send
#include <stdio.h> //perror
#include <stdlib.h> //exit
#include <arpa/inet.h> //htons, inet_addr
#include <netinet/in.h> //inet_addr
#include <string.h> //bzero
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 3533
#define MAX_RECV_BUF 256
#define MAX_buff 256

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

/*________________________________Presionar cualquier tecla para continuar_______________*/
int press_any_key( void ){
	printf("Presione cualquier tecla para continuar...");
	int ch;
	struct termios oldt, newt;

	tcgetattr ( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );

	return ch;
}

int main(){
   
	int clientfd, r, opcion, num_registro, v, flag, cont, i, size;
	struct dogType *perro;
	struct sockaddr_in client;
	socklen_t sizefd;
	char cadena[32];
	char* nombre;
   char* gedit = "gedit ";
   char* ext = ".txt";
   char* x;
	char buff[MAX_RECV_BUF];
	ssize_t rcvd_bytes;
	int histC;
	
	/*___________________________configurarcion del cliente___________________________*/
	//crear un socket
	clientfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (clientfd == -1){ perror("error socket"); exit(-1); }
	//configurar el socket
	client.sin_family = AF_INET;
	client.sin_port = htons(PORT);
	client.sin_addr.s_addr= inet_addr("127.0.0.1"); 
	bzero(client.sin_zero, 8);
	sizefd = sizeof(struct sockaddr_in);
	r = connect(clientfd, (struct sockaddr*)&client, sizefd);
	if(r==-1){ perror("Error connecting to server"); exit(-1); }

	/*___________________________Enviar mensajes al servidor__________________________*/
	do{
	   //fflush(stdout);
		printf("________________________________________________________________________________\n");
		printf("1. Ingresar registro. \n");
		printf("2. Ver registro. \n");
		printf("3. Borrar registro. \n");
		printf("4. Buscar registro. \n");
		printf("5. Salir. \n\n");
		printf("6. Mostrar todos. \n");
		printf("Elija una de las opciones: ");

		
		scanf("%i", &opcion);
		printf("\n");

		//send   
   		r = send(clientfd, &opcion, 4, 0);
		if(r != 4){ perror("Error en send 1"); exit(-1); }

		switch(opcion){
			case 1:

				perro = malloc(sizeof(struct dogType));				

				//Pedir datos de dogType por pantalla
				printf("Nombre: "); scanf("%s", perro->nombre);
				printf("Tipo: "); scanf("%s", perro->tipo);
				printf("Edad: "); scanf("%i", &perro->edad);
				printf("Raza: "); scanf("%s", perro->raza);
				printf("Estatura: "); scanf("%i", &perro->estatura);
				printf("Peso: "); scanf("%f", &perro->peso);
				printf("Sexo: "); scanf(" %c", &perro->sexo);
				perro->next = 0;
				perro->prev = 0;

				r = send(clientfd, perro, sizeof(struct dogType), 0);
				if(r == -1){ perror("error send 2"); exit(-1); }

				free(perro);
            			printf("Su mascota ha sido ingresada exitosamente.\n");
				getchar();
				press_any_key();
				
			break;

			case 2:
				perro = malloc(sizeof(struct dogType));	

				r = recv(clientfd, &num_registro, sizeof(num_registro), 0);
				if(r == -1){ perror("error recv 1"); exit(-1); }
				printf("Numero de registros presentes: %i\n", num_registro-1);
				
				printf("Ingrese el numero de registro que desea ver: ");
				scanf("%i", &v); printf("\n");
				r = send(clientfd, &v, sizeof(v), 0);
				if(r == -1){ perror("error send 3"); exit(-1); }

				r = recv(clientfd, &flag, sizeof(flag), 0);
				if(r == -1){ perror("error recv 2"); exit(-1); }
				
				if(flag == 1){
					r = recv(clientfd, perro, sizeof(struct dogType), 0);
					if(r == -1){ perror("error recv 3"); exit(-1); }
					
					r = recv(clientfd, &size, sizeof(int), 0);
					if(r == -1){ perror("error recv size"); exit(-1); }
					
					char nombreArch[40];
					strcpy(nombreArch, perro->nombre);
					strcat(nombreArch, ".txt");
					//printf("%s", nombreArch);


					//-------------Recibir archivo--------------------------------------------
					//Abre archivo.
					histC = open(nombreArch, O_CREAT | O_WRONLY, 0777);
					if(histC == NULL){ perror("error al crear archivo histC"); exit(-1); }
				
					do{
					   rcvd_bytes = recv(clientfd, buff, MAX_buff, 0);
					   if(rcvd_bytes == -1){ perror("error send 4"); exit(-1); }
					   //printf("rcvd_bytes: %i\n", rcvd_bytes);
					   
					   r = write(histC, buff, rcvd_bytes);
					   if(r == -1){ perror("error write"); exit(-1); }
					   //printf("write: %i\n", r);

					   size -= rcvd_bytes;
					}while(rcvd_bytes < size);
				
					close(histC);
					//------------------------------------------------------------------------
							
					//Nombre del archivo a ejecutar.
					x = malloc(strlen(gedit)+strlen(nombre)+strlen(ext)+5);
					strcpy(x, gedit);
					strcat(x, perro->nombre);
					strcat(x, ext);

					//Muestra los datos y libera espacio.
					imprimir(perro);                 

					//Ejecutar gedit con archivo.
					system(x);

					free(x);

					printf("Apertura de la hitoria clinica exitosa. \n");
					  
					//Enviar archivo de vuelta---------------------------
					histC = fopen(nombreArch, "r+");
					if(histC == NULL){ perror("error al crear archivo histC"); exit(-1); }
					 
					//Calcula el tamaño del archivo
					fseek(histC, 0L, SEEK_END);
					size = ftell(histC);
					
					
					//Envía el tamaño del archivo
					r = send(clientfd, &size, sizeof(int), 0);
					if(r == -1){ perror("error send size"); exit(-1); }				

					fclose(histC);
					
					
					histC = open(nombreArch, O_RDONLY);
					if(histC == NULL){ perror("error al crear archivo histC"); exit(-1); }
					
					ssize_t read_bytes, /* bytes read from local file */
							sent_bytes; /* bytes sent to server socket */
					
					do{
						read_bytes = read(histC, &buff, MAX_buff);
						if(read_bytes == -1){ perror("error read_bytes"); exit(-1); }					   
						//printf("read_bytes: %i\n", read_bytes);

						sent_bytes = send(clientfd, buff, read_bytes, 0);
						//printf("sent_bytes: %i\n", sent_bytes);
						if(sent_bytes == -1){ perror("error send chunk"); exit(-1); }

						size -= sent_bytes;
					}while(sent_bytes < size);

					//Cierra archivo.
					close(histC);
					//---------------------------------------------
					r = remove(nombreArch);
					if(r == -1){ perror("error remove"); exit(-1); }
				}
				else{
					printf("Numero de registro no valido.\n");				
				}
			
				free(perro);
				getchar();
				press_any_key();
			break;

			case 3:
				r = recv(clientfd, &num_registro, sizeof(num_registro), 0);
				if(r == -1){ perror("error recv 4"); exit(-1); }
				printf("Numero de registros presentes: %i\n", num_registro-1);

				printf("Ingrese el numero de registro que desea Borrar: ");
				scanf("%i", &v);
				r = send(clientfd, &v, sizeof(v), 0);
				if(r == -1){ perror("error send 4"); exit(-1); }

				r = recv(clientfd, &flag, sizeof(flag), 0);
				if(r == -1){ perror("error recv 5"); exit(-1); }
				if(flag == 1){
					printf("Borrado exitoso. \n");
				}
				else{
					printf("Numero de registro no valido.\n");				
				}

				getchar();
				press_any_key();
			break;

			case 4:
				printf("Ingrese el nombre de la mascota que desea buscar: ");
				scanf("%s", cadena);
				printf("\n");
				r = send(clientfd, cadena, sizeof(cadena), 0);
				if(r == -1){ perror("error send 5"); exit(-1); }

				r = recv(clientfd, &flag, sizeof(flag), 0);
				if(r == -1){ perror("error recv 6"); exit(-1); }
				
				if(flag == 1){
		
					r = recv(clientfd, &cont, sizeof(cont), 0);
					if(r == -1){ perror("error recv 7"); exit(-1); }
				
					for(i = 0; i < cont; i++){
						perro = malloc(sizeof(struct dogType));	
						r = recv(clientfd, perro, sizeof(struct dogType), 0);
						if(r == -1){ perror("error recv 8"); exit(-1); }
						imprimir(perro);
						free(perro);
						printf("\n");
					}	

					printf("Impresion exitosa.\n");
				}
				else{
					printf("No existen registros asociados al nombre %s\n", cadena);
				}
			
				getchar();
				press_any_key();
			break;

			case 5:
			break;

			default:
				printf("Opcion no valida, intenta de nuevo.\n");
			break;
		}
		printf("\n\n");
		

	}while(opcion != 5);
	
	/*_______________________________________Finalizar_____________________________________*/
	printf("fin del cliente \n");
	
	//liberar recursos
	close(clientfd);
	

}
