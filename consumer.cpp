#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cstring>

struct Buffer {
    char commodities[40][11]; // needs to end with null
    double prices[40];
    int in;
    int out;
};

void sem_wait(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, -1, 0}; // -1 do dec
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, 0}; // +1 do inc
    semop(sem_id, &op, 1);	
}

int main(char *argv[]) {	
    int size = std::stoi(argv[1]);
    size = 40;
	
    double p0[11];
    double avgPrices[11];
    double p1[11];
    double p2[11];				
    double p3[11];
    double p4[11];
		
    int i;
    for(i = 0; i < 11; i++){		
        p0[i] = 0;
        avgPrices[i] = 0;
        p1[i] = 0;
        p2[i] = 0;
        p3[i] = 0;
        p4[i] = 0;
    };			

    // key_t shm_key = ftok("/tmp", 8207);
    // key_t sem_key = ftok("/tmp", 8003);	
    key_t shm_key = 3003;
    key_t sem_key = 4004;			
	

    int shm_id = shmget(shm_key, sizeof(Buffer), 0666 | IPC_CREAT);
		
    auto *buffer = static_cast<Buffer *>(shmat(shm_id, nullptr, 0));
    buffer->out = 0;
	
    int sem_id = semget(sem_key, 3, 0666 | IPC_CREAT);

    char names[11][11] = {"ALUMINIUM","COPPER","COTTON","CRUDEOIL","GOLD","LEAD","MENTHAOIL","NATURALGAS","NICKEL","SILVER","ZINC"};	

    printf(" Commodity  | Price     | AvgPrice   \n");
    printf("------------------------------------\n");
	
    for (i = 0; i < 11 ; i++){
        std::cout << names[i] << " | " << p0[i] << " | " << avgPrices[i]<< " \n";
    };	
	
    while (true){

        sem_wait(sem_id, 2); // semwait on full to consume
        sem_wait(sem_id, 0); // semwait on mutex to access cs
	
        for (i = 0; i < 11 ; i++){
            // std::cout << names[i] << buffer->commodities[buffer->out];	
            if (strcmp(names[i],buffer->commodities[buffer->out]) == 0){
                p4[i] = p3[i];
                p3[i] = p2[i];
                p2[i] = p1[i];		
                p1[i] = p0[i];
                p0[i] = buffer->prices[buffer->out];
                avgPrices[i] = (p0[i] + p1[i] + p2[i] + p3[i] + p4[i]) / 5;
            };	
        };		

        printf("\e[1;1H\e[2J");
        printf(" Commodity  | Price     | AvgPrice   \n");
        printf("------------------------------------\n");

        for (i = 0; i < 11 ; i++){
            if (strcmp(names[i],buffer->commodities[buffer->out]) == 0){
                char* arrow = " ";
                if (p0[i] > p1[i]) {	
                    arrow = "↑";

                    std::cout << buffer->commodities[buffer->out] << " | " << "\033[;32m" << p0[i] << arrow << "\033[0m" << " | " << "\033[;32m" << avgPrices[i] << arrow << "\033[0m \n";				
                } else if (p0[i] < p1[i]) {
                    arrow = "↓";

                    std::cout << buffer->commodities[buffer->out] << " | " << "\033[;31m" << p0[i] << arrow << "\033[0m" << " | " << "\033[;31m" << avgPrices[i] << arrow << "\033[0m \n";				
                }	
            }
            else{	
                std::cout << names[i] << " | " << p0[i] << " | " << avgPrices[i]<< " \n";
                }		
        };	
	
        buffer->out = (buffer->out + 1) % size;		

        sem_signal(sem_id, 0);
        sem_signal(sem_id, 1);

        usleep(2000);

    };	

    shmdt(buffer);		
    return 0;
}
