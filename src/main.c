/* Based on code from https://community.amd.com/t5/opencl/clutil-a-library-for-making-opencl-as-easy-to-use-as-cuda/m-p/388306/ */

#define CL_TARGET_OPENCL_VERSION 200

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <CL/cl.h>
#include "ethash.h"

// connect string
//char connect_json[] = "{\"id\":\"0\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"agent\":\"EthereumStratum/1.0.0\",\"login\":\"341T2ew7f3aZafjfmhQWRhJ3VaA3pcHCCL\",\"pass\":\"x\"}"
const char miner_subscribe[] = "{\"id\":0,\"method\":\"mining.subscribe\",\"params\":[\"phiMiner/0.0.1\",\"EthereumStratum/1.0.0\"]}\n";
const char job_template[] = "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"getjobtemplate\",\"params\":null}\n";
const char extranonce[]="{\"id\":1,\"method\":\"mining.extranonce.subscribe\",\"params\":[]}\n";
const char miner_authorize[] = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"341T2ew7f3aZafjfmhQWRhJ3VaA3pcHCCL.phiMiner\",\"X\"]}\n";


// Submit strings
const char submit_json0[] = "{\"id\":";
const char submit_json1[] = ",\"method\":\"mining.submit\",\"params\":[\"341T2ew7f3aZafjfmhQWRhJ3VaA3pcHCCL.phiMiner\","; // nonce
const char submit_json2[] = ","; //pow
const char submit_json3[] = "]}\n";

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Prototypes
void gendag(uint start, const uint16* _Cache, uint16* _DAG0, uint16* _DAG1, uint light_size, cl_command_queue queue, cl_kernel kernel, size_t *global, size_t *local);
void *receiveMessage(void *sock_desc);
void *sendMessage(void *sock_desc);
int stringSplit(char *text, char seperator, int leading, char (*args)[1024]);
//void mine(struct SearchResults* g_output, __constant uint2 const* g_header, ulong8 const* _g_dag0, ulong8 const* _g_dag1, uint dag_size, ulong start_nonce, ulong target);

#define MAIN

#ifdef MAIN
int main(int argc, char** argv)
{
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue commandQueue;
    cl_mem buffer;
    cl_program program;
    cl_kernel kernel;
    unsigned int length = 0;
	uint start=0;
	uint16 _Cache;
	uint16 _DAG0;
	uint16 _DAG1;
	uint light_size;
	unsigned int ip;
	const char *source=kernelSource;
	    int fd;
    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in serveraddr;
    memset( &serveraddr, 0, sizeof(serveraddr) );
    inet_pton(AF_INET,argv[1],&ip);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons( 3353 );              
    serveraddr.sin_addr.s_addr = ip;  

    //Connect to remote server
    if (connect(fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        perror("Connect failed. Error");
        return 1;
    }

    printf("Connected to %s\n", argv[1]);
    int *new_sock;
    new_sock = malloc(sizeof(int));
    *new_sock = fd;

    //Initialization
    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    commandQueue = clCreateCommandQueue(context, device, 0, &err);
    program = clCreateProgramWithSource(context, 1, &source, 0, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "testkernel", &err);

    //Allocate memory    
    //buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(array), NULL, &err);

    size_t global;
    size_t local = 64;
   
    global = length % local == 0 ? length : (length / local + 1) * local;

	pthread_t send_thread, receive_thread;
    pthread_create(&send_thread, NULL, sendMessage, (void *) new_sock);
    pthread_create(&receive_thread, NULL, receiveMessage, (void *) new_sock);

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

	//gendag(start, &_Cache, &_DAG0, &_DAG1, light_size, commandQueue, kernel, &global, &local);

	// connect();

	// gendag();
	
	// minerloop();
	
		// newjob();
			// mine();
				// verify();
					// submit();
		

    

    //Copy data back
    //err = clEnqueueReadBuffer(commandQueue, buffer, CL_TRUE, 0, sizeof(array), array, 0, NULL, NULL);
	close(fd);
    
}
#endif

void *sendMessage(void *sock_desc) {
    //Send some data
    if ( send(*((int *) sock_desc),miner_subscribe, sizeof(miner_subscribe),0) >= 0)
    	printf("CLIENT: %s",miner_subscribe);
    else
    	printf("CLIENT: Miner failed to subsribe");
    
    sleep(1);
    
    /*if ( send(*((int *) sock_desc),job_template, sizeof(job_template),0) >= 0)	
		printf("CLIENT: %s",job_template);
	else
		printf("CLIENT: Auth Failure");*/
    sleep(1);
    
    if ( send(*((int *) sock_desc),extranonce, sizeof(extranonce),0) >= 0)	
		printf("CLIENT: %s",extranonce);
	else
		printf("CLIENT: Auth Failure");
    
    sleep(1);
    
    if ( send(*((int *) sock_desc),miner_authorize, sizeof(miner_authorize),0) >= 0)	
		printf("CLIENT: %s",miner_authorize);
	else
		printf("CLIENT: Auth Failure");
	
	while(1)
	{
		sleep(10);
	}
		
}

void *receiveMessage(void *sock_desc) {
	char reply[2000];
	char split[16][1024];
	char split2[16][1024];
	int d=0;
	
    while (1) {
        
        if (recv(*((int *) sock_desc), reply, 2000, 0) < 0) {
            printf("SERVER: recv failed");

        }
	        
	    if ( reply[0] != '\0' )
    	{
	    	//Receive a reply from the server
	        printf("SERVER: %s",reply);
	        /*stringSplit(reply, ':', 0, split);
	        if ( split[3][0] != '\0' )
	        {
		       stringSplit(split[3],',',1,split2);
			}*/
	        		    
        	memset(reply,'\0',sizeof(reply));
        	reply[0]='\0';
	    }
        
    	
	    sleep(1);
	}
    
}

void gendag(uint start, const uint16* _Cache, uint16* _DAG0, uint16* _DAG1, uint light_size, cl_command_queue queue, cl_kernel kernel, size_t *global, size_t *local)
{
	cl_int err;

     //Actually call the kernel
    err = clSetKernelArg(kernel, 0, sizeof(start), &start);
    err = clSetKernelArg(kernel, 1, sizeof(_Cache), &_Cache);
    err = clSetKernelArg(kernel, 2, sizeof(_DAG0), &_DAG0);
    err = clSetKernelArg(kernel, 3, sizeof(_DAG1), &_DAG1);
    err = clSetKernelArg(kernel, 4, sizeof(light_size), &light_size);
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, local, 0, NULL, NULL);
    
}

int stringSplit(char *text, char seperator, int leading, char (*args)[1024])
{
	int splits=0;
	int pos=0;

	for(int i=leading; i < strlen(text); i++)
	{


		if ( *(text+i) == seperator )
		{
			splits++;
			pos=0;

			if ( seperator == '\r' )
			{
				i++;
			}
		}

		if ( *(text+i) != seperator )
		{
			args[splits][pos]=*(text+i);
			pos++;
		}
	}


	return(splits);
}

/*
void mine(struct SearchResults* g_output, __constant uint2 const* g_header, ulong8 const* _g_dag0, ulong8 const* _g_dag1, uint dag_size, ulong start_nonce, ulong target)  
{
    err = clSetKernelArg(kernel, 0, sizeof(SearchResults), &g_output);
    err = clSetKernelArg(kernel, 1, sizeof(g_header), &g_header);
    err = clSetKernelArg(kernel, 2, sizeof(_g_dag0), &_g_dag0);
    err = clSetKernelArg(kernel, 3, sizeof(_g_dag1), &_g_dag1);
    err = clSetKernelArg(kernel, 4, sizeof(dag_size), &dag_size);
    err = clSetKernelArg(kernel, 5, sizeof(start_nonce), &start_nonce);
    err = clSetKernelArg(kernel, 6, sizeof(target), &target);
    
    err = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
}*/

/*

//
void verify()
{

}

void submit()
{

}

*/



