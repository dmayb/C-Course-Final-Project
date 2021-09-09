#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SpreaderDetectorParams.h"

#define REQUESTED_ARGS_COUNT 2
#define READ_MODE "r"
#define WRITE_MODE "w"
#define TRUE 1
#define FALSE 0
#define MAX_LINE_SIZE 1025
#define F_ERROR 2 // just a code
#define F_EOF 3  // just a code
#define SUCCESS 0
#define INPUT_FILE_ERR_MSG "Error in input files.\n"
#define OUTPUT_FILE_ERR_MSG "Error in output file.\n"
#define USAGE_MSG "Usage: ./SpreaderDetectorBackend <Path to People.in> <Path to Meetings.in>\n"
#define PEOPLE_LINE_FORMAT "%s %lu %f"
#define MEETING_LINE_FORMAT "%lu %lu %f %f"
#define PERSON_FIELD_NUM 3
#define MEETING_FIELD_NUM 4

/**
 * @struct Person
 * @brief: struct for holding people data: Id, Name, Age and chance of infection
 */
typedef struct Person
{
	char *Name;
	unsigned long Id;
	float ChanceOfInfection;
} Person;

/**
 * @struct Meeting
 * @brief: struct for keeping a meeting's details: Infector's Id, Infected's Id, Distace between
 * the 2 people, and the duration of the meeting
 */
typedef struct Meeting
{
	unsigned long infectorId;
	unsigned long infectedId;
	float Distance;
	float Time;

} Meeting;

/**
 * gets 2 people and compare their ids (for sorting small to large)
 * @param a
 * @param b
 * @return <0 if a.id<b.id, >0 if a.id>b.id, 0 if a.id==b.id
 */
int comparePeopleById(const void *a, const void *b)
{
	const unsigned long idA = ((Person *) a)->Id;
	const unsigned long idB = ((Person *) b)->Id;
	if (idA == idB)
	{
		return 0;
	}
	if (idA > idB)
	{
		return 1;
	}
	return -1;
}

/**
 * comparing 2 persons by chance. for sorting large to small
 * @param a
 * @param b
 * @return
 */
int comparePeopleByChance(const void *a, const void *b)
{
	const float chanceA = ((Person *) a)->ChanceOfInfection;
	const float chanceB = ((Person *) b)->ChanceOfInfection;
	if (chanceA - chanceB == 0)
	{
		return 0;
	}
	if (chanceA > chanceB)
	{
		return -1;
	}
	return 1;
}

/**
 * read a line from filePt using fgets, and check for reading errors and EOF
 * if a line was successfully read, replace newline char with '\0'
 * @param filePt - file descriptor to read from
 * @param line - read into this line
 * @return F_ERROR if reading failed, F_EOF if reached end of file, SUCCESS if all worked.
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
 * free people array and the people's name (which was dynamically allocated)
 * @param people
 * @param peopleCount
 */
void freePeople(Person **people, const int peopleCount)
{
	for (int i = 0; i < peopleCount; ++i)
	{
		free((*people)[i].Name);
	}
	free(*people);
	*people = NULL;
}

/**
 * get a line from person.in and parses it into Person struct
 * @param line
 * @param person
 * @return TRUE if worked, FALSE otherwise
 */
int parsePersonLine(const char line[MAX_LINE_SIZE], Person *const person)
{
	float age = 0;
	char name[MAX_LINE_SIZE] = {};
	int fieldsNum = sscanf(line, PEOPLE_LINE_FORMAT, name, &(person->Id), &age);
	if (fieldsNum != PERSON_FIELD_NUM || strlen(name) <= 0)
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
 * Read the People.in file and create People array
 * @param peopleFile
 * @param people
 * @return the number of people
 */
int readPeopleFile(FILE *peopleFile, Person **people)
{
	Person *tempPtr = NULL; // for reallocating
	char line[MAX_LINE_SIZE] = {}; // for reading
	int peopleCount = 0;
	int sizeToAlloc = 1;
	int parseStatus = TRUE;

	int readStatus = readLineSafe(peopleFile, line);
	while (readStatus == SUCCESS) // will stop on either on F_ERROR or F_EOF
	{
		tempPtr = (Person *) realloc(*people, sizeToAlloc * (sizeof(Person))); // try reallocating
		sizeToAlloc++;
		if (tempPtr == NULL) // allocation failed
		{
			freePeople(people, peopleCount);
			fclose(peopleFile);
			fprintf(stderr, STANDARD_LIB_ERR_MSG);
			exit(EXIT_FAILURE);
		}
		*people = tempPtr;
		parseStatus = parsePersonLine(line, (*people + peopleCount));
		if (!parseStatus) // parsing failed
		{
			fprintf(stderr, STANDARD_LIB_ERR_MSG);
			freePeople(people, peopleCount);
			fclose(peopleFile);
			exit(EXIT_FAILURE);
		}
		peopleCount++;
		readStatus = readLineSafe(peopleFile, line); // read another line
	}

	if (readStatus == F_ERROR) // reading failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(people, peopleCount);
		fclose(peopleFile);
		exit(EXIT_FAILURE);
	}
	return peopleCount;
}

/**
 * gets the path to a people file, tries to open it and then reads it and parses it into an array
 * @param filepath
 * @param people
 * @return number of people that were read and parsed
 */
int openAndReadPeopleFile(const char *filepath, Person **people)
{
	FILE *peopleFile = NULL;

	peopleFile = fopen(filepath, READ_MODE);
	if (peopleFile == NULL)
	{
		fprintf(stderr, INPUT_FILE_ERR_MSG);
		exit(EXIT_FAILURE);
	}

	int peopleCount = readPeopleFile(peopleFile, people);

	if (fclose(peopleFile)) // fclose failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(people, peopleCount);
		exit(EXIT_FAILURE);
	}

	return peopleCount;
}

/**
 * binary search in sorted by ids people array
 * @param id
 * @param people
 * @param peopleCount
 * @return the person that belongs to the id
 */
Person *findPersonById(const unsigned long id, const Person *people, const int peopleCount)
{
	Person personToSearch = {};
	personToSearch.Id = id;
	Person *ptrToPerson = &personToSearch;
	Person *found = (Person *) bsearch((void *) ptrToPerson, (void *) people, peopleCount,
									   sizeof(Person), comparePeopleById);
	return found;
}

/**
 * read the first line of the meetings.in file and return the id in it (the spreader id)
 * @param meetingsFile
 * @param line
 * @return spreader id
 */
unsigned long getSpreaderId(FILE *meetingsFile, char line[MAX_LINE_SIZE])
{
	const int readStatus = readLineSafe(meetingsFile, line);
	if (readStatus != SUCCESS)
	{
		return readStatus;
	}
	const unsigned long id = strtoul(line, NULL, 10);
	return id;
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
 * Calculate infection chance
 * @param meeting
 * @return  infection chance
 */
float crna(const Meeting *meeting)
{
	const float time = meeting->Time;
	const float dist = meeting->Distance;
	return (time * MIN_DISTANCE) / (dist * MAX_TIME);
}

/**
 * read meeting file and calculate infection state for each person in a meeting
 * @param meetingsFile
 * @param spreadTree
 * @param people
 * @param peopleCount
 * @return
 */
void readMeetingsFile(FILE *meetingsFile, Person *people, const int peopleCount)
{
	char line[MAX_LINE_SIZE] = {};

	const unsigned long spreaderId = getSpreaderId(meetingsFile, line);
	if (spreaderId == F_EOF) // file is empty
	{
		return;
	}

	if (spreaderId == F_ERROR)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}

	Person *spreader = findPersonById(spreaderId, people, peopleCount);
	spreader->ChanceOfInfection = 1; // he's infected

	Meeting meeting = {};
	int parseStatus = TRUE;
	int readStatus = readLineSafe(meetingsFile, line); // read a meeting line
	Person *infected = NULL;
	Person *infector = NULL;
	while (readStatus == SUCCESS)// 2m*logn=O(mlogn) (when n- number of ppl, m - number of meetings)
	{
		parseStatus = parseMeetingLine(line, &meeting);
		if (!parseStatus)
		{
			fprintf(stderr, STANDARD_LIB_ERR_MSG);
			freePeople(&people, peopleCount);
			fclose(meetingsFile);
			exit(EXIT_FAILURE);
		}
		infected = findPersonById(meeting.infectedId, people, peopleCount); // O(logn)
		infector = findPersonById(meeting.infectorId, people, peopleCount); // O(logn)
		infected->ChanceOfInfection = crna(&meeting) * infector->ChanceOfInfection;
		readStatus = readLineSafe(meetingsFile, line);
	}

	if (readStatus == F_ERROR)
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		fclose(meetingsFile);
		exit(EXIT_FAILURE);
	}
}

/**
 * open and read meetings file. for each person in it - calculate the infection chance
 * @param filepath
 * @param people
 * @param peopleCount
 */
void readMeetingsAndUpdateInfectionChances(const char *filepath, Person *people,
										   const int peopleCount)
{
	FILE *meetingsFile = NULL;
	meetingsFile = fopen(filepath, READ_MODE);
	if (meetingsFile == NULL)
	{
		fprintf(stderr, INPUT_FILE_ERR_MSG);
		freePeople(&people, peopleCount);
		exit(EXIT_FAILURE);
	}

	// read the meetings file and update infection chance according to the meetings
	readMeetingsFile(meetingsFile, people, peopleCount);

	if (fclose(meetingsFile)) // fclose failed
	{
		fprintf(stderr, STANDARD_LIB_ERR_MSG);
		freePeople(&people, peopleCount);
		exit(EXIT_FAILURE);
	}
}

/**
 * Writes the medical recommendation for each person, according to their infection chance.
 * the data will be sorted from highest infection chance to lowest
 * @param people
 * @param peopleCount
 * @return TRUE on success FALSE otherwise
 */
int writeDataToOutputFile(const Person *people, const int peopleCount)
{
	FILE *outputFile = fopen(OUTPUT_FILE, WRITE_MODE);
	if (outputFile == NULL)
	{
		return FALSE;
	}
	for (int j = 0; j < peopleCount; ++j)
	{
		if (people[j].ChanceOfInfection >= MEDICAL_SUPERVISION_THRESHOLD)
		{
			fprintf(outputFile, MEDICAL_SUPERVISION_THRESHOLD_MSG, people[j].Name, people[j].Id);
		}
		else if (people[j].ChanceOfInfection >= REGULAR_QUARANTINE_THRESHOLD)
		{
			fprintf(outputFile, REGULAR_QUARANTINE_MSG, people[j].Name, people[j].Id);
		}
		else
		{
			fprintf(outputFile, CLEAN_MSG, people[j].Name, people[j].Id);

		}
	}

	if (fclose(outputFile)) // fclose failed
	{
		return FALSE;
	}
	return TRUE;
}

/**
 * @brief: The SpreadDetectorBackend program receives two files: People & Meetings.
 * it than reads each file and calculates each person that has appeared in the meeting file, he's
 * chance of getting infected. People who are not in the meeting file will be considered with a 0
 * chance of getting infected. The program calssifies each person according to it's infection
 * chance severity and prints into a file a medical recommendation for each person, sorted from
 * highest infection chance to the lowest.
 * The program does its work for time complexity of O((m+n)logn), where n is the number of people
 * and m is the number of meetings.
 * @param argc
 * @param argv
 * @return EXIT_SUCCESS upon success, EXIT_FAILURE otherwise
 */
int main(int argc, char *argv[])
{
	if (argc - 1 != REQUESTED_ARGS_COUNT) // args count is wrong
	{
		fprintf(stderr, USAGE_MSG);
		exit(EXIT_FAILURE);
	}

	Person *people = NULL;
	const int peopleCount = openAndReadPeopleFile(argv[1], &people);

	if (peopleCount == 0) // means people file is empty, nothing to do, exit with success
	{
		freePeople(&people, peopleCount);
		const int writeStatus = writeDataToOutputFile(people, peopleCount); // will create empty
		// file
		if (!writeStatus)
		{
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
	qsort((void *) people, peopleCount, sizeof(Person), comparePeopleById); //sort for better
	// search time - O(nlogn)

	readMeetingsAndUpdateInfectionChances(argv[2], people, peopleCount);

	//sort by infection chance
	qsort((void *) people, peopleCount, sizeof(Person), comparePeopleByChance);

	const int writeStatus = writeDataToOutputFile(people, peopleCount);

	freePeople(&people, peopleCount);

	if (!writeStatus)
	{
		fprintf(stderr, OUTPUT_FILE_ERR_MSG);
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
