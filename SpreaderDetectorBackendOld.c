#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SpreaderDetectorParams.h"

#define REQUESTED_ARGS_COUNT 2
#define READ_MODE "r"
#define TRUE 1
#define FALSE 0
#define MAX_LINE_SIZE 1025
#define F_ERROR -1
#define F_EOF -2
#define SUCCESS 0
#define FAIL 1
#define INPUT_FILE_ERR_MSG "Error in input files.\n"
#define OUTPUT_FILE_ERR_MSG "Error in output file.\n"
#define PEOPLE_LINE_FORMAT "%s %lu %f"
#define MEETING_LINE_FORMAT "%lu %lu %f %f"
#define PERSON_FIELD_NUM 3
#define MEETING_FIELD_NUM 4
#define PEOPLE_FILE "People.in"
#define MEETINGS_FILE "Meetings.in"


/**
 *
 */
typedef struct Person
{
	char *Name;
	long Id;
	float ChanceOfInfection;
} Person;

/**
 *
 */
typedef struct Meeting
{
	long infectorId;
	long infectedId;
	float Distance;
	float Time;

} Meeting;

/**
 *
 */
typedef struct Node
{//todo: change to capital leeters
	Person *Data;
	struct Node **sons;
	struct Node *Parent;
	int sonsCount;

	Meeting *meetingsData;
} Node;

/**
 *
 */
typedef struct Tree
{
	Node *root;
	int size;
} Tree;


/**
 *
 * @param a
 * @param b
 * @return
 */
int idComparator(const void *a, const void *b)
{
	if ((*(long *) a - *(long *) b) == 0)
	{
		return 0;
	}
	if ((*(long *) a - *(long *) b) > 0)
	{
		return 1;
	}
	return -1;
}

/**
 *
 * @param a
 * @param b
 * @return
 */
int comparePeopleById(const void *a, const void *b)
{
	long idA = ((Person *) a)->Id;
	long idB = ((Person *) b)->Id;
	return idComparator((void *) &idA, (void *) &idB);
}

/**
 *
 * @param a
 * @param b
 * @return
 */
int comparePeopleByChance(const void *a, const void *b)
{
	float chanceA = ((Person *) a)->ChanceOfInfection;
	float chanceB = ((Person *) b)->ChanceOfInfection;
	return idComparator((void *) &chanceA, (void *) &chanceB);
}

/**
 *
 * @param a
 * @param b
 * @return
 */
int compareChance(const void *a, const void *b)
{
	if ((*(float *) a - *(float *) b) == 0)
	{
		return 0;
	}
	if ((*(float *) a - *(float *) b) > 0)
	{
		return -1;
	}
	return 1;
}

/**
 * check if a file is empty
 * @param file
 * @return TRUE if empty, FALSE otherwise
 */
int isFileEmpty(FILE *const file)
{
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET); // put it back to the start of the file
	if (size == 0)
	{
		return TRUE;
	}
	return FALSE;
}

/**
 * open the input file and check:
 * 		if open failed, print to stderr and exit program
 * 		if file is empty, print to stderr and exit program
 * @param filename - file to open
 * @param filePt - open into this pointer
 */
void safeOpenInputFile(char const *const filename, FILE **filePt)
{
	*filePt = fopen(filename, READ_MODE);
	if (*filePt == NULL) // todo: has to be null? could be something else but still bad?
	{
		fprintf(stderr, INPUT_FILE_ERR_MSG);
		exit(EXIT_FAILURE);
	}
}

/**
 *
 * @param filePt
 * @param line
 * @return
 */
int readLineSafe(FILE *const filePt, char line[MAX_LINE_SIZE])
{
	if (fgets(line, MAX_LINE_SIZE, filePt) == NULL)
	{
		if (ferror(filePt))
		{
			return F_ERROR;
		}

		if (feof(filePt))
		{
			return F_EOF;
		}
	}
	if (strlen(line) > 0 && line[strlen(line) - 1] == '\n') // replace '\n' with '\0'
	{
		line[strlen(line) - 1] = '\0';
	}
	return SUCCESS;
}

/**
 * free people array
 * @param people
 * @param peopleCount
 */
void freePeople(Person **people, int peopleCount)
{
	for (int i = 0; i < peopleCount; ++i)
	{
		free((*people)[i].Name);
	}
	free(*people);
	*people = NULL;
}

/**
 *
 * @param line
 * @param person
 * @return
 */
int parsePersonLine(const char line[MAX_LINE_SIZE], Person *const person)
{
	float age = 0;
	char name[MAX_LINE_SIZE] = {};
	int fieldsNum = sscanf(line, PEOPLE_LINE_FORMAT, name, &(person->Id), &age);// todo:inserts \0?
	if (fieldsNum != PERSON_FIELD_NUM || strlen(name) <= 0) //todo: second
	{
		return FALSE;
	}
	person->Name = (char *) calloc(strlen(name) + 1, sizeof(char));
	if (!person->Name)
	{
		return FALSE;
	}

	strcpy(person->Name, name);
	person->ChanceOfInfection = 0;
	return TRUE;
}

/**
 *
 * @param peopleFile
 * @param people
 * @return
 */
int readPeopleFile(FILE *peopleFile, Person **people)
{
	Person *temp = NULL;
	char line[MAX_LINE_SIZE] = {};
	int peopleCount = 0;
	int sizeToAlloc = 1;
	int parseStatus = 1;

	int readStatus = readLineSafe(peopleFile, line);
	while (readStatus == SUCCESS)
	{
		temp = (Person *) realloc(*people, sizeToAlloc * (sizeof(Person)));
		sizeToAlloc++;
		if (temp == NULL)
		{
			freePeople(people, peopleCount);
			fclose(peopleFile);
			fprintf(stderr, STANDARD_LIB_ERR_MSG);
			exit(EXIT_FAILURE);
		}
		*people = temp;
		parseStatus = parsePersonLine(line, (*people + peopleCount));
		if (!parseStatus)
		{
			fprintf(stderr, STANDARD_LIB_ERR_MSG);
			freePeople(people, peopleCount);
			fclose(peopleFile);
			exit(EXIT_FAILURE);
		}
		peopleCount++;
		readStatus = readLineSafe(peopleFile, line);
	}

	if (readStatus == F_ERROR)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG); //todo: what to print
		freePeople(people, peopleCount);
		fclose(peopleFile);
		exit(EXIT_FAILURE);
	}
	return peopleCount;
}

/**
 *
 * @param meetingsFile
 * @param line
 * @return
 */
long getSpreaderId(FILE *meetingsFile, char line[MAX_LINE_SIZE])
{
	int readStatus = readLineSafe(meetingsFile, line);
	if (readStatus != SUCCESS)
	{
		return readStatus;
	}
	long id = strtol(line, NULL, 10); //todo: is ok to put null?
	return id;
}

/**
 * creates a new node wit given id, and links it to the person holding that id
 * @param id
 * @param people
 * @param peopleCount
 * @return
 */
Node *createNode(const long id, const Person *people, const int peopleCount)
{
	Node *node = (Node *) calloc(1, sizeof(Node));
	if (node == NULL)
	{
		return NULL;
	}
	Person personToSearch = {};
	personToSearch.Id = id;
	Person *ptr = &personToSearch;
	Person *found = (Person *) bsearch((void *) ptr, (void *) people, peopleCount,
									   sizeof(Person), comparePeopleById);
	node->Data = found;
	node->sonsCount = 0;
	node->sons = NULL;
	node->meetingsData = NULL;
	node->Parent = NULL;
	return node;
}

/**
 *
 * @param meetingsFile
 * @param line
 * @param people
 * @param peopleCount
 * @return
 */
Node *getSpreadRoot(FILE *meetingsFile, char line[MAX_LINE_SIZE], Person *people, int peopleCount)
{
	long spreaderId = getSpreaderId(meetingsFile, line);

	if (spreaderId == F_ERROR)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}

	if (spreaderId == F_EOF)
	{//todo: creating a file with all zeros
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		fclose(meetingsFile);
		exit(EXIT_SUCCESS);
	}

	Node *root = createNode(spreaderId, people, peopleCount);
	if (root == NULL)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}
	return root;
}

/**
 * free all the tree data
 * @param root
 */
void freeSpreadTree(Node **root)
{
	if (root == NULL || *root == NULL)
	{
		return;
	}
	for (int i = 0; i < (*root)->sonsCount; ++i)
	{
		freeSpreadTree((&(*root)->sons[i])); // recusivley free sons of sons ...
		free((*root)->sons[i]); // free actual son

	}
	free((*root)->meetingsData); // free meetings
	free((*root)->sons); // free sons array
	free(*root); // free root
	*root = NULL;
}

/**
 * parse a meeting line <infector-id> <infected-id> <distance> <time>
 * @param line
 * @param meeting
 * @return TRUE on success FALSE otherwise
 */
int parseMeetingLine(const char line[MAX_LINE_SIZE], Meeting *meeting)
{
	int fieldNum = sscanf(line, MEETING_LINE_FORMAT, &meeting->infectorId, &meeting->infectedId,
						  &meeting->Distance, &meeting->Time);
	if (fieldNum != MEETING_FIELD_NUM)
	{
		return FALSE;
	}
	return TRUE;
}

/**
 * reads untill new id in the meeting file. it stores all the infected people infected by curId
 * (which is the infeector). it stores the infecteds a the infector's sons
 * @param curId
 * @param meetingsFile
 * @param line
 * @param infector
 * @param people
 * @param peopleCount
 * @return
 */
int readUntilNewId(long curId, FILE *meetingsFile, char line[MAX_LINE_SIZE], Node *infector,
				   const Person *people, int peopleCount, long *nextId)
{
	int readStatus = SUCCESS; // the line given is valid.
	Meeting meeting = {};
	int parseStatus = TRUE;
	Node *infected = NULL;
	Node **tempNodePtr = NULL;
	Meeting *tempMeetingPtr = NULL;
	while (readStatus == SUCCESS) // keep reading lines until status changes
	{
		parseStatus = parseMeetingLine(line, &meeting); //parse the line into a meeting struct
		if (!parseStatus) // parse failed
		{
			return FAIL;
		}
		if (meeting.infectorId != curId) // we reached a new id, exit function
		{
			*nextId = meeting.infectorId; // send out the new id
			return SUCCESS;
		}
		// at this point parsing successed and the id is still the same. so read the infected id's
		infected = createNode(meeting.infectedId, people, peopleCount);
		if (infected == NULL) // something failed (memory allocation)
		{
			return FAIL;
		}
		// allocate new memory for the new infected data
		tempNodePtr = (Node **) realloc(infector->sons, (infector->sonsCount + 1) * sizeof(Node *));
		tempMeetingPtr = (Meeting *) realloc(infector->meetingsData, (infector->sonsCount + 1) *
																	 sizeof(Meeting));
		if (tempNodePtr == NULL || tempMeetingPtr == NULL) // allocations failed
		{
			free(tempNodePtr);
			free(tempMeetingPtr);
			return FAIL;
		}
		// update infected data
		infected->Parent = infector;
		infector->sons = tempNodePtr; // the new infected is added as a son to the infector
		infector->sons[infector->sonsCount] = infected;
		infector->meetingsData = tempMeetingPtr;
		infector->meetingsData[infector->sonsCount] = meeting; // adding the infector-infected meet
		infector->sonsCount++;

		readStatus = readLineSafe(meetingsFile, line); // read another line
	}
	return readStatus; // could be F_ERROR or F_EOF
}

/**
 *
 * @param prevInfector
 * @param nextId
 * @return
 */
Node *findNextInfector(const Node *prevInfector, const long nextId)
{
	// the next one is either a son, a sibling, a niece or a cousin
	//search in sons
	for (int i = 0; i < prevInfector->sonsCount; ++i)
	{
		if (prevInfector->sons[i]->Data->Id == nextId)
		{
			return prevInfector->sons[i];
		}
	}
	//search siblings
	for (int j = 0; j < prevInfector->Parent->sonsCount; ++j)
	{
		if (prevInfector->Parent->sons[j]->Data->Id == nextId)
		{
			return prevInfector->Parent->sons[j];
		}
		// search nieces
		for (int i = 0; i < prevInfector->Parent->sons[j]->sonsCount; ++i)
		{
			if (prevInfector->Parent->sons[j]->sons[i]->Data->Id == nextId)
			{
				return prevInfector->Parent->sons[j]->sons[i];
			}
		}
	}
	// search cousins:
	for (int k = 0; k < prevInfector->Parent->Parent->sonsCount; ++k)
	{
		for (int i = 0; i < prevInfector->Parent->Parent->sons[k]->sonsCount; ++i)
		{
			if (prevInfector->Parent->Parent->sons[k]->sons[i]->Data->Id == nextId)
			{
				return prevInfector->Parent->Parent->sons[k]->sons[i];
			}
		}
	}

	return NULL; // if couldnt find infector (wont happen on valid meeting file)
}

/**
 *
 * @param meetingsFile
 * @param spreadTree
 * @param people
 * @param peopleCount
 * @return
 */
Node* readMeetingsFile(FILE *meetingsFile, Person *people, int peopleCount)
{
	char line[MAX_LINE_SIZE] = {};

	Node *root = getSpreadRoot(meetingsFile, line, people, peopleCount); // the spreader
	long spreaderId = root->Data->Id;

	int lineStatus = readLineSafe(meetingsFile, line); // read a meeting line
	if (lineStatus == F_ERROR) // reading failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		freeSpreadTree(&root);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}

	if (lineStatus == F_EOF)
	{
		return root;
	}

	Node *infector = root;
	long nextId = 0;
	int readStatus = readUntilNewId(spreaderId, meetingsFile, line, infector, people,
									peopleCount, &nextId);
	while (readStatus == SUCCESS)
	{
		infector = findNextInfector(infector, nextId);
		readStatus = readUntilNewId(nextId, meetingsFile, line, infector, people,
									peopleCount, &nextId);
	}

	if (readStatus == F_ERROR || readStatus == FAIL)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG); //todo: what to print
		freePeople(&people, peopleCount);
		freeSpreadTree(&root);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}
	return root;
}


/**
 *
 * @param time
 * @param dist
 * @return
 */
float crna(const Meeting *meeting)
{
	float time = meeting->Time;
	float dist = meeting->Distance;
	return (time * MIN_DISTANCE) / (dist * MAX_TIME);
}

/**
 *
 * @param infector
 * @param prevChance
 */
void calcRecursive(Node *infector, float prevChance)
{
	Node *infected = NULL;
	Node **sons = infector->sons;
	float infectionChance = 0;
	for (int i = 0; i < infector->sonsCount; ++i)
	{
		infected = sons[i];
		infectionChance = crna(&infector->meetingsData[i]);
		infected->Data->ChanceOfInfection = prevChance * infectionChance;
		calcRecursive(infected, prevChance * infectionChance);
	}
}

/**
 *
 * @param spreadTree
 */
void calculateInfectionChance(Node *infector)
{
//	Node *infector = spreadTree->root;
	infector->Data->ChanceOfInfection = 1;
	float startingChance = 1;
	calcRecursive(infector, startingChance);
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
	if (argc - 1 != REQUESTED_ARGS_COUNT)
	{
		fprintf(stderr, argv[0]); //todo: what to print?
		exit(EXIT_FAILURE);
	}

	FILE *peopleFile = NULL;
//	char *peopleFileName = strcat(argv[1], PEOPLE_FILE);
//	safeOpenInputFile(argv[1], &peopleFile);
	safeOpenInputFile("2_people.in", &peopleFile);
	Person *people = NULL;
	int peopleCount = readPeopleFile(peopleFile, &people);
	if (fclose(peopleFile)) // fclose failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		exit(EXIT_FAILURE);
	}

	if (peopleCount == 0) // nothing to do, exit with success
	{
		//todo: create emptyfile
		freePeople(&people, peopleCount);
		exit(EXIT_SUCCESS);
	}

	qsort((void *) people, peopleCount, sizeof(Person), comparePeopleById);


	FILE *meetingsFile = NULL;
	//safeOpenInputFile(argv[2], &meetingsFile);
	safeOpenInputFile("2_meeting.in", &meetingsFile);


	Node *spreadTreeRoot = readMeetingsFile(meetingsFile, people, peopleCount);

	if (fclose(meetingsFile)) // fclose failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		exit(EXIT_FAILURE);
	}


	calculateInfectionChance(spreadTreeRoot);
	qsort((void *) people, peopleCount, sizeof(Person), comparePeopleByChance);


	for (int j = 0; j < peopleCount; ++j)
	{
//		if (people[j].ChanceOfInfection >= MEDICAL_SUPERVISION_THRESHOLD)
//		{
//			printf(MEDICAL_SUPERVISION_THRESHOLD_MSG, people[j].Name, people[j].Id);
//		}
//		else if (people[j].ChanceOfInfection >= REGULAR_QUARANTINE_THRESHOLD)
//		{
//			printf(REGULAR_QUARANTINE_MSG, people[j].Name, people[j].Id);
//		}
//		else
//		{
//			printf(CLEAN_MSG, people[j].Name, people[j].Id);
//
//		}
		printf("%s %lu %f\n", people[j].Name, people[j].Id, people[j].ChanceOfInfection);

	}


	freePeople(&people, peopleCount);
	freeSpreadTree(&spreadTreeRoot);
	return EXIT_SUCCESS;
}
