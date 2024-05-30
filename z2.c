#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

enum STATE {BUTTON, B0, B1, B2, B3};
typedef enum STATE STATE;

pthread_t t1;
pthread_t t2;
int gotsignal = 0;

void sighandler(int signo){

	if(signo == SIGIO) gotsignal = 1;

}

void clr(){
	system("clear");
}

unsigned long read_timer(){
	FILE *fp;
	char *str;
	size_t len = 8;
	int i = 0;

	unsigned long long tenano = 0;
	unsigned long ms = 0;

	fp = fopen ("/dev/timer", "r");

	str = (char *)malloc(len+1); 
	getline(&str, &len, fp); 
	fclose(fp);


	for(i=0; i<8; ++i){
	       	tenano+=(unsigned long long)str[i] << (8*i);
	//	printf("tenano %d: %llu", i, tenano);
	}
	free(str);

	ms = tenano/100000;
	
	return ms;
}

void write_time(unsigned long time){

	unsigned int ms = 0;
	unsigned int millis = 0;
	unsigned int sek = 0;
	unsigned int minutes = 0;
	unsigned int hours = 0;

	ms = time;
	
	hours = ms / 3600000;
	ms -= hours*3600000;
	
	minutes = ms /60000;
	ms -= minutes*60000;
	
	sek = ms / 1000;
	ms = ms - sek*1000;

	millis = ms;

	printf("%u:%u:%u:%u \n", hours, minutes, sek, millis);

}

void write_timer(unsigned long time, char state){
	FILE *fp;

	fp = fopen ("/dev/timer", "w");
	fprintf(fp, "%c, %lu", state, time);


//	printf("write_timer ms: %c, %d \n", state, time);
	fclose(fp);

}

void read_buttons(bool * button){
	FILE *fp;
	char *str;
	char tval1,tval2,tval3,tval4;
	size_t num_of_bytes = 6;

	//Citanje vrednosti tastera
	fp = fopen ("/dev/button", "r");

	str = (char *)malloc(num_of_bytes+1); 
	getline(&str, &num_of_bytes, fp); 

	fclose(fp);


	button[0] = str[2] - 48;
	button[1] = str[3] - 48;
	button[2] = str[4] - 48;
	button[3] = str[5] - 48;
	free(str);

//	printf("Vrednosti button: %d %d %d %d \n",button[0],button[1],button[2],button[3]);
	
	sleep(1);
}

void *read_keyboard(void *argv);
void * app(void *argv);




int main ()
{
	unsigned long time = 0;
	struct sigaction action;

	clr();
	write_timer(0, 'p');

	time = read_timer();
	write_time(time);

	//asinhroni signal:
	int fd = open("/dev/timer", O_RDWR|O_NONBLOCK);
	memset(&action, 0, sizeof(action));
	action.sa_handler = sighandler;
	sigaction(SIGIO, &action, NULL);

	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);	

	//thread:
	pthread_create(&t1, NULL, read_keyboard, NULL);
	pthread_create(&t2, NULL, app, NULL);
	
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	gotsignal = 0;
	close(fd);


return 0;
}





void *read_keyboard(void *argv){
	int i = 0;
	char key;
	unsigned long time = 0;
	
	while(1) {
	
		system("stty raw"); 
		if(getchar()){
			system(" stty cooked");
			clr();
			time = read_timer();
			write_time(time);
		}

		usleep(1000);
	}
}


void * app(void *argv){

	//pom za tastere:
	bool button[4];

	//pom za BUTTON:
	bool last_state = 0;
	bool current_state = 1;

	//pom za B0:
	int start_pause = 1;

	unsigned long time = 0;
	STATE state = BUTTON;

while(1)
	{
	
		switch(state){

			case BUTTON:
				usleep(1000);

				if(gotsignal) state = B3;

 				read_buttons(button);
				
				if(current_state != last_state){
				
					if(button[0]) state = B0;
						   
					if(button[1]) state = B1; 
						   
					if(button[2]) state = B2; 
						   
					if(button[3]) state = B3;
				
					current_state = last_state;
				}

				if(!(button[0] || button[1] || button[2] || button[3])) current_state = !last_state; 
			break;

			case B0:
				time = read_timer();
			//	write_time(time);

				if(start_pause){
				       	write_timer(time, 's');
					start_pause = !start_pause;

				}else{
					write_timer(time, 'p');
					start_pause = !start_pause;
				}

				state = BUTTON;
			break;

			case B1:
				time = read_timer();
			//	write_time(time);

				//printf("B1\n");

				if(time >= 10000) time-=10000;

			//	write_time(time);

				if(!start_pause){ 
					write_timer(time, 's');
				}else{
					write_timer(time, 'p');
				}

				state = BUTTON;
			break;
			
			case B2:
				time = read_timer();
			//	write_time(time);

				//printf("B2\n");

				time+=10000;

			//	write_time(time);

				if(!start_pause){ 
					write_timer(time, 's');
				}else{
					write_timer(time, 'p');
				}

				state = BUTTON;
			break;

			case B3:
				clr();

				sleep(1);

				pthread_cancel(t1);
				pthread_exit(NULL);
			break;

		}
	}


}


