#include <iostream>
#include <cstring>
#include <random>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct Buffer {
    char commodities[40][11];	
    double prices[40];		
    int in;	
    int out;	
};

void sem_wait(int sem_id, int sem_num) {	
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);	
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(sem_id, &op, 1);
}
	
int main(char *argv[]) {

    // key_t shm_key = ftok("/tmp", 8207);
    // key_t sem_key = ftok("/tmp", 8003);
    key_t shm_key = 3003;
    key_t sem_key = 4004;		

    const char* commodity = argv[1];
    double mean = std::stod(argv[2]);			
    double dev = std::stod(argv[3]); 	
    int sleep = std::stoi(argv[4]);
    int size = std::stoi(argv[5]);
    size = 40;


    int shm_id = shmget(shm_key, sizeof(Buffer),IPC_CREAT | 0666);		

    auto *buffer = static_cast<Buffer *>(shmat(shm_id, nullptr, 0));
    buffer->in = 0;
		
    int sem_id = semget(sem_key, 3, 0666 | IPC_CREAT);	

    if (sem_id == -1) {
        sem_id = semget(sem_key, 3, 0666);
    } else {
        semctl(sem_id, 0, SETVAL, 1);	
        semctl(sem_id, 1, SETVAL, size);
        semctl(sem_id, 2, SETVAL, 0);		
    }

    std::default_random_engine generator(time(nullptr));
    std::normal_distribution<double> distribution(mean, dev);		
    double price;

    while (true) {
        	
        price = distribution(generator);

        struct timespec timenano;
        clock_gettime(CLOCK_REALTIME, &timenano);
        time_t sec = timenano.tv_sec;		
        struct tm* time = localtime(&sec);

        std::cerr << "["<< time->tm_hour<<":"<< time->tm_min<<":"<< time->tm_sec<<"."<<timenano.tv_nsec << "] " << commodity << ": generating a new value " << price << "\n";
		
        sem_wait(sem_id, 1);
        
        std::cerr << "["<< time->tm_hour<<":"<< time->tm_min<<":"<< time->tm_sec<<"."<<timenano.tv_nsec << "] " << commodity << ": trying to get mutex on shared buffer \n";
        
        sem_wait(sem_id, 0) ;	

        std::cerr << "["<< time->tm_hour<<":"<< time->tm_min<<":"<< time->tm_sec<<"."<<timenano.tv_nsec << "] " << commodity << ": placing " << price << " in shared buffer\n";		
				

        strncpy(buffer->commodities[buffer->in], commodity);
        buffer->commodities[buffer->in][10] = '\0';
        buffer->prices[buffer->in] = price;			
        buffer->in = (buffer->in + 1) % size;
			
        sem_signal(sem_id, 0);
        sem_signal(sem_id, 2);
        
        std::cerr << "["<< time->tm_hour<<":"<< time->tm_min<<":"<< time->tm_sec<<"."<<timenano.tv_nsec << "] " << commodity << ": sleeping for " << interval << " ms\n";
        
        usleep(sleep * 1000);				
    }
				
    shmdt(buffer);
    return 0;				
}
