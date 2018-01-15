#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#define MAXLEN 100
#define CROSS 5

//define the node structure for the queue
typedef struct qNode Node;
struct qNode
{
	char *driver;
	char *direction;
	int arrival;
	int arrivalNum;
	Node *next;	
};

//function declarations
void initialize();
void enqueue(Node *newCar);
void dequeue();
void sortArrivals();
int length();
int canCross(int direction);
Node *readCar();
Node *getFirstNode();
void *car();
void arriveBridge();
void exitBridge();

//global pointers to the start(head) and end(tail) of the queue
Node *head;
Node *tail;

//global variable declarations
int timer;
int bridgeDirection;
int numOnBridge;
int waitingSouth;
int waitingNorth;
pthread_mutex_t lock1;
pthread_cond_t bridgeSouth;
pthread_cond_t bridgeNorth;
pthread_cond_t timeWait;
//sem_t *bridge;

int main()
{
	int i;

	//initialize bridge
	timer = 0;
	numOnBridge = 0;
	waitingNorth = 0;
	waitingSouth = 0;

	//chew up and ignore the header
	char header[MAXLEN];
	fgets(header, sizeof(header), stdin);

	//initialize queue pointers
	initialize();

	//load the cars into the queue
	int count = 0;
	Node *newCar;
	newCar = readCar();
	while(newCar != NULL)
	{
		enqueue(newCar);
		count = count + 1;
		newCar = readCar();
	}

	//sort the queue based on arrival time
	sortArrivals();

	//declare the cars
	pthread_t carThread[count];

	//create the cars
	for(i = 0; i < count; i++)
	{
		pthread_create(&carThread[i], NULL, car, NULL);
	}

	//wait for cars to finish
	for(i = 0; i < count; i++)
	{
		pthread_join(carThread[i], NULL);
	}
}

//sets the head and tail pointers to null and indicates to the user that the pointers are ready to use
void initialize()
{
	head = tail = NULL;
}

//loads a job into the end of the queue
void enqueue(Node *newCar)
{
	//set up the job to be added
	char driver[MAXLEN];
	char direction[MAXLEN];
	int arrival;
	Node *temp = NULL;
	temp = (Node*)malloc(sizeof(Node));
	temp->driver = newCar->driver;
	temp->direction = newCar->direction;
	temp->arrival = newCar->arrival;
	temp->next = NULL;

	if(tail == NULL)	//the queue must be empty, so set both head and tail to temp
	{
		head = temp;
		tail = temp;
	}
	else			//the queue is not empty, so add the job to the end of the queue
	{
		tail->next = temp;
		tail = temp;	
	}
}

//removes a job from the front of the queue
void dequeue()
{
	//set up the job to be removed
	Node *rem = head;
	if(head == NULL)	//the queue is empty
	{ 
		head = tail = NULL;
	}
	else			//queue is not empty, so remove the job at the start of the queue
	{
		head = head->next;
		free(rem);
		rem = NULL;
	}
}

Node* getFirstNode()
{
	//set up the job to be returned
	Node *firstNode;
	firstNode = head;
	if(head == NULL)	//the queue is empty
	{
		printf("queue is empty\n");
	}
	else			//the queue is not empty, so return the first job in the queue
	{
		return firstNode;
	}
}

//return the length of the queue
int length()
{
	int length = 0;
	Node *temp = head;

	if(tail == NULL)	//the queue must be empty
	{
		return length;
	}
	else			//the queue is not empty, so increment length and continue until the end is reached
	{
		while(temp)
		{
			length++;
			temp = temp->next;
		}
	}
	return length;
}

//reads in a job from standard input
Node *readCar()
{
	//set up the job to be added
	char driver[MAXLEN];
	char direction[MAXLEN];
	int arrival;
	Node *newCar = NULL;

	/*check that we are not at the end of the file and that the format is correct.
	if so, set up and return the job*/
	if(!feof(stdin) && (3 == scanf("%s %s %d", driver, direction, &arrival)))
	{
		newCar = (Node*)malloc(sizeof(Node));
		newCar->driver = malloc(strlen(driver)+1);
		strcpy(newCar->driver, driver);
		newCar->direction = malloc(strlen(direction)+1);
		strcpy(newCar->direction, direction);
		newCar->arrival = arrival;
		newCar->next = NULL;
	}

	return newCar;
}

void sortArrivals()
{
	Node *start = head;
	Node *comp;
	Node *swap; 
	int counter = 0;
	int i;

	if(start == NULL)	//list is empty
	{
		return;
	}
	while(start->next)
	{
		counter = 0;
		swap = start;
		comp = start->next;
		//loop as long as there is another node
		while(comp)
		{
			if(comp->arrival < swap->arrival)	//the job has arrived
			{
				swap = comp;
			}
			if(swap != start)	//we have changed swap
			{
				Node *push = start;
				char *tempDriver1 = start->driver;
				char *tempDirection1 = start->direction;
				int tempArrival1 = start->arrival;
				char *tempDriver2;
				char *tempDirection2;
				int tempArrival2;
				start->driver = swap->driver; 
				start->direction = swap->direction;
				start->arrival = swap->arrival;

				//loop as long as we have not reached swap
				while(push != swap)
				{
					if(push->next != swap)	//the next node is not swap
					{
						tempDriver2 = push->next->driver;
						tempDirection2 = push->next->direction;
						tempArrival2 = push->next->arrival;
						push = push->next;
						push->driver = tempDriver1;
						push->direction = tempDirection1;
						push->arrival = tempArrival1;
						tempDriver1 = tempDriver2;
						tempDirection1 = tempDirection2;
						tempArrival1 = tempArrival2;
					}
					else	//the next node is swap
					{
						push->next->driver = tempDriver1;
						push->next->arrival = tempArrival1;
						push->next->direction = tempDirection1;
						push = push->next;
					}
				}
			}
			comp = comp->next;
		}
		start = start->next;
	}
}

//car
void *car()
{
	char *driver;
	int direction;
	int arrival;
	pthread_mutex_lock(&lock1);
	Node *carVals = getFirstNode();
	driver = carVals->driver;
	dequeue();
	pthread_mutex_unlock(&lock1);
	arrival = carVals->arrival;
	if(strcmp(carVals->direction, "N") == 0)	//car is heading north
	{
		direction = 0;
	}
	else	//car is heading south
	{
		direction = 1;
	}
	//car arrives at bridge
	arriveBridge(direction, arrival);
	//car exits bridge
	exitBridge(direction);
}

//car arrives at bridge
void arriveBridge(int direction, int arrival)
{
	pthread_mutex_lock(&lock1);
	if(arrival > timer)	//has not arrived yet
	{
		while(arrival > timer)	//wait until it arrives
		{
			pthread_cond_wait(&timeWait, &lock1);
		}
	}
	if(!canCross(direction))	//cannot cross
	{
		if(direction == 0)	//check direction
		{
			waitingNorth = waitingNorth + 1;
		}
		else	//check direction
		{
			waitingSouth = waitingSouth + 1;
		}

		while(!canCross(direction))	//wait until it can cross
		{

			if(direction == 0)	//check direction
			{
				pthread_cond_wait(&bridgeNorth,&lock1);
			}
			else	//check direction
			{
				pthread_cond_wait(&bridgeSouth, &lock1);
			}
		}

		if(direction == 0)	//check direction
		{
			waitingNorth = waitingNorth - 1;
		}
		else	//check direction
		{
			waitingSouth = waitingSouth - 1;
		}
	}
	numOnBridge = numOnBridge + 1;
	bridgeDirection = direction;
	pthread_mutex_unlock(&lock1);	
}

void exitBridge(int direction)
{
	pthread_mutex_lock(&lock1);
	numOnBridge = numOnBridge - 1;
	if(numOnBridge > 0)	//there are still cars on the bridge
	{
		if(direction == 0)	//check direction
		{
			pthread_cond_signal(&bridgeNorth);
		}
		else	//check direction
		{
			pthread_cond_signal(&bridgeSouth);
		}
	}
	else	//there are no cars left on the bridge
	{
		if(direction == 0)	//check direction
		{
			if(waitingSouth > 0)	//since car was heading north and bridge is now empty, check if any waiting on the south direction
			{
				pthread_cond_signal(&bridgeSouth);
			}
			else	//nobody waiting on either side, so finish
			{
				pthread_cond_signal(&bridgeNorth);
			}
		}	
		else
		{
			if(waitingNorth > 0)	//since car was heading south and bridge is now empty, check if any waiting on the north direction
			{
				pthread_cond_signal(&bridgeNorth);
			}
			else	//nobody waiting on either side, so finish
			{
				pthread_cond_signal(&bridgeSouth);
			}
		}
	}
	pthread_mutex_unlock(&lock1);
}

//checks if the bridge is available for a car to cross in the given direction
int canCross(int direction)
{
	if(numOnBridge == 0)	//no cars on bridge, so bridge is free
	{
		return 1;
	}
	else if(direction == bridgeDirection && numOnBridge < 3)	//there are cars on the bridge, but less than three and heading in the same direction, so bridge is free
	{
		return 1;
	}
	else	//bridge is not free
	{
		return 0;
	}
}
