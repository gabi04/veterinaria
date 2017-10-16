#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

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

struct perroHash{
	int primerPerro;
	int ultimoPerro;
};

//Funcion de imprimir mascotas.
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

//Devuelve el codigo hash correspondiente a un nombre dado.
unsigned int funcion_hash(char *s) {
	unsigned int h;
	unsigned const char *us;

	//to lower case
	int i;
	for(i = 0; i < strlen(s); i++)
		s[i] = tolower(s[i]);
	
	us = (unsigned const char *) s;
	h = 0;
	while(*us != '\0') {
		h = (h * 256 + *us) % 1187;
		us++;
	}

	return h;
}

int main(){
	//variables
	FILE *nombres;
	FILE *archivo;
	FILE *tabla;
	int r;
	int i = 0;
	int fin;
	int aleatorio;
	srand(time(NULL));
	//int num_estructuras = 10000000;
	int num_estructuras = 4;
	char str[32];
	char str2[32];
	int h, id;
	int pos, linea;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	struct perroHash *p_hash;
	p_hash = malloc(sizeof(struct perroHash));
	struct dogType *perro_temp;
	perro_temp = malloc(sizeof(struct dogType)); //Separar espacio.
	struct dogType *mascota;
	mascota = malloc(sizeof(struct dogType));

	//Crea array con 10 razas de perro
	const char *razasPerro[] = {"pastor aleman", "greyhound", "husky siberiano", "akita inu", "chihuahua", "corgi gales", "schnauzer",  		"pinscher", "gran danes", "yorkshire"};

	//Crea el archivo en donde se almacenan las 10000000 estructuras
	archivo = fopen("dataDogs.dat", "a+");
	if(archivo == NULL){ perror("Error al crear archivo dataDogs: a+"); exit(-1); }
	fclose(archivo);

	//Crear tablaHash para no tener el array tabla_hash[10000][1] en memoria
	tabla = fopen("tablaHash.dat", "a+");
	if(tabla == NULL){perror("Error al crear archivo tablaHash a+: "); exit(-1);}
	fclose(tabla);

	//Asignarle un tamaño determinado a tablaHash
	r = truncate("tablaHash.dat", sizeof(struct perroHash)*10000);

	//Abrir archivo con 1000 nombres de mascotas
	nombres = fopen("nombresMascotas.txt", "r");
	if(nombres == NULL){
		perror("Error al crear archivo: ");
		exit(-1);
	}

	//abre el archivo en donde se almacena la tabla hash
	tabla = fopen("tablaHash.dat", "r+");
	if(tabla == NULL){ perror("Error al crear archivo tablaHash: r+"); exit(-1); }

	//abre el archivo en donde se almacenan las estructura
	archivo = fopen("dataDogs.dat", "r+");
	if(archivo == NULL){ perror("Error al crear archivo dataDogs: r+"); exit(-1); }


	//Generar las 10000000 estructuras aleatoria y guardarlas en el archivo
	while(num_estructuras--){

		//mueve el cursor al inicio de los archivos
		fseek(nombres, 0L, SEEK_SET);

		//elegir un nombre aleatorio del archivo
		aleatorio = rand() % 1000;
		//aleatorio = rand() %2;
		while (aleatorio>0) {
			read = getline(&line, &len, nombres);
			aleatorio--;
		}

		//asigna el nombre leido del archivo a la mascota
		//printf("line %s %i \n", line, strlen(line));
		memset(mascota->nombre,0,strlen(mascota->nombre));

		for(i=0; i<strlen(line)-1; i++) {
			mascota->nombre[i] = line[i];
		}

        //printf("mascota->nombre %s %i \n", mascota->nombre, strlen(mascota->nombre));


		//elegir una raza aleatoria
		strcpy(str2, razasPerro[num_estructuras%10]);

		//llena la estructura con datos
		aleatorio = rand() % 4;
		if(1){

			strcpy(mascota->tipo, "perro");
			mascota->prev = 0;
			mascota->next = 0;
			//strcpy(mascota->nombre, str);
			strcpy(mascota->nombre, "cesar");
			strcpy(mascota->raza, str2);
			mascota->edad = rand() % 20;
			mascota->estatura = rand() % 100 + 10;
			mascota->peso = rand() % 40 + 30;
			if(aleatorio % 2 == 0)
				mascota->sexo = 'M';
			else
				mascota->sexo = 'H';
		}

		//calcula la posicion en que se va a guardar la nueva estructura
		fseek(archivo, 0L, SEEK_END);
		pos = ftell(archivo) + 1;
		//printf("posicion: %ld\n", pos);

		//Obtiene un codigo hash correspondiente al nombre de la mascota.
		h = funcion_hash(mascota->nombre);
		//printf("Hash %s: %i \n", mascota->nombre, h);

        //busca la estructura perroHash h en tablaHash
        fseek(tabla, h*sizeof(struct perroHash), SEEK_SET);
        r = fread(p_hash, sizeof(struct perroHash), 1, tabla);

		//Inserta la posicion del perro en la hash table si es el primero con este codigo (1)
		//Si hay una colicion lo inserta al final de una linked list(2)
		/*(1)*/if(p_hash->primerPerro == 0){

			p_hash->primerPerro = pos;

		}/*(2)*/else{

            //Ubica la ultima mascota con el hash
            linea = p_hash->ultimoPerro - 1;
            fseek(archivo, linea, SEEK_SET);
            r = fread(perro_temp, sizeof(struct dogType) , 1 , archivo);

			//Actualiza los datos de perro_temp y perro
			perro_temp->next = pos;
			mascota->prev = p_hash->ultimoPerro;

			fseek(archivo, linea, SEEK_SET);
			r = fwrite(perro_temp , sizeof(struct dogType) , 1 , archivo);
			if(r == 0){ perror("perro_temp no se puede escribir"); exit(-1); }
		}

		//guarda el perro nuevo al final del archivo
		fseek(archivo, 0, SEEK_END);
		r = fwrite(mascota , sizeof(struct dogType) , 1 , archivo);
		if(r == 0){ perror("perro se puede escribir "); exit(-1); }

        p_hash->ultimoPerro = pos;

        //Escribe la nueva pareja p_hash en tabla
        fseek(tabla, h*sizeof(struct perroHash), SEEK_SET);
        r = fwrite(p_hash , sizeof(struct perroHash) , 1 , tabla);

	}


	//libera el espacio de mascota y perro_temp
	free(mascota);
   	free(perro_temp);

	//cerrar todos los archivos abiertos
	fclose(nombres);
	fclose(archivo);
	fclose(tabla);



	//imprime todas las estructuras guardadas
/*
	printf("\n\n");
	mascota = malloc(sizeof(struct dogType));
	archivo = fopen("dataDogs.dat", "a+");
	if(archivo == NULL){ perror("Error al crear archivo dataDogs: a+"); exit(-1); }
	fseek(archivo, SEEK_SET, 0);
	while(fread(mascota, sizeof(struct dogType), 1, archivo) != 0){
		imprimir(mascota);
	}
	free(mascota);
	fclose(archivo);
*/

}

