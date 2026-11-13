#include<iostream>
#include<unistd.h>

int main(){
	pid_t process_id = getpid();

	std::cout << "OSBench - Linux System Call Test  \n";
	std::cout << "Process ID : " << process_id << "\n";


	return 0;

}
