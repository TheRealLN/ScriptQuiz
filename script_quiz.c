// ScriptQuiz - A program that will ask randomly generated questions based off a script
//              given in a specified format

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Has to be at least 20
#define LINE_SIZE 500
// The number of attempts an otherwise infinite loop will take
// (Should be at least equal to maximum number of responses)
#define ATTEMPTS 20

// Macro to allow using LINE_SIZE in format string
// https://stackoverflow.com/questions/12844117/printing-defined-constants
#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

// Contains a quote with the line_num of the quote, the author of the quote and the next author/quote
typedef struct Question
{
	int line_num;
	char author[LINE_SIZE];
	char quote[LINE_SIZE];
	char next_author[LINE_SIZE];
	char next_quote[LINE_SIZE];
} Question;

// Function prototypes
void err (char *str);
char toLower (char c);
int isWhiteSpace (char c);
int isAlphaNumeric (char c);
char cleanResponse (char *str);
int numOfLines (char *filename);
void compileScript (char **script, char *filename, int lineCount);
void freeScript(char **script, int lineCount);
void removeTrailingWhitespace (Question *q);
Question *createQuestion (char **script, int lineCount, int line_num);
Question *findQuestion (char **script, int lineCount, int diffLineNum);
int isValidAnswerChoice (Question *q, Question **q_choices, int i);
int askQuestion (char **script, int lineCount, int question_num, int num_responses, int diffFlag);
void playQuiz (char **script, int lineCount, char *name, int num_questions, int num_responses, int diffFlag);


// Will display an error message and then exit the program
void err (char *str)
{
	printf("\nError: %s\n\n", str);
	exit(1);
}

// Creates a lowercase version of the char provided
char toLower (char c)
{
	char r = c;
	
	if (c >= 'A' && c <= 'Z')
		r = c + 32;

	return r;
}

// Tests if a char is alphanumeric
int isAlphaNumeric (char c)
{
	if ( toLower(c) >= 'a' && toLower(c) <= 'z')
		return 1;
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

// Tests for whitespace
int isWhiteSpace (char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// Converts a user response string into an answer, '\0' (0) if no answer could be determined
char cleanResponse (char *str)
{
	int i;
	
	if (str == NULL)
		return '\0';
	
	// Finds the first alphanumeric character and returns it as the answer
	for (i = 0; i < LINE_SIZE, str[i] != '\0'; i++)
		if (isAlphaNumeric(str[i]))
			return str[i];
	
	// Could not find any alphanumeric character
	return '\0';
}

// Returns the number of lines in a file
int numOfLines (char *filename)
{
	char c;
	int count = 1;
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;
	
	// Counts number of lines in file
	// https://www.geeksforgeeks.org/c-program-count-number-lines-file/
	for (c = getc(fp); c != EOF; c = getc(fp)) 
        if (c == '\n') // Increment count if this character is newline 
            count = count + 1;
	
	fclose(fp);
	return count;
}

// Writes the contents of a script file into a double array
void compileScript (char **script, char *filename, int lineCount)
{
	char tmpstr[LINE_SIZE];
	int i = 0;
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
		return;
	
	if (script == NULL)
		return;
	
	// Writes each line of the script into script
	while (1)
	{
		if (fgets(tmpstr, sizeof(tmpstr), fp) == NULL)
			break;
		
		// Removes trailing newline and other weird ending characters
		// https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
		tmpstr[strcspn(tmpstr, "\r\n")] = 0;
		
		// Copies string into script
		script[i] = malloc(LINE_SIZE * sizeof(char));
		strcpy(script[i], tmpstr);
		i++;
	}
	
	fclose(fp);
	return;
}

// Deallocates the contents of the script array
void freeScript(char **script, int lineCount)
{
	int i;
	
	if (script == NULL)
		return;
	
	for (i = 0; i < lineCount; i++)
		if (script[i] != NULL)
			free(script[i]);
	free(script);
	
	return;
}

// Removes trailing whitespace from a quetion
void removeTrailingWhitespace (Question *q)
{
	int i, j = -1;
	
	if (q == NULL)
		return;
	
	// Removes trailing whitespace from the quote
	for (i = 0; (q->quote)[i] != '\0'; i++)
		if (!isWhiteSpace((q->quote)[i]))
			j = i;
	(q->quote)[j+1] = '\0';
	
	// Removes trailing whitespace from the next quote
	j = -1;
	for (i = 0; (q->next_quote)[i] != '\0'; i++)
		if (!isWhiteSpace((q->next_quote)[i]))
			j = i;
	(q->next_quote)[j+1] = '\0';
	
	return;
}

// Creates the question, returns NULL if the question couldn't be created from the given line number
Question *createQuestion (char **script, int lineCount, int line_num)
{
	Question *q;
	char quote[500];
	int i = 0, j = 0, k = 0;
	
	if (script == NULL || script[line_num] == NULL)
		return NULL;
	
	// Tests if line_num is valid
	if (line_num < 0 || line_num >= lineCount)
		return NULL;
	
	// Tests if the line itself is a quote
	if (!isAlphaNumeric(script[line_num][0]) && (script[line_num][0] != '\t' || script[line_num][0] != ' '))
		return NULL;
	
	// Allocates memory for the question
	q = malloc(sizeof(Question));
	if (q == NULL)
		err("Memory could not be allocated for the question.");
	
	// Saves the line number that the quote is taken from
	q->line_num = line_num;
	
	// Quote is not the first line of text by author... (\t..\tQUOTE)
	if (script[line_num][0] == '\t' || script[line_num][0] == ' ')
	{
		// Iterate through all the whitespace
		while (i < LINE_SIZE && isWhiteSpace(script[line_num][i]))
			i++;
		
		// Creates the quote by getting rid of all the previous whitespace
		for (; i < LINE_SIZE && script[line_num][i] != '\0'; i++)
			q->quote[j++] = script[line_num][i];
		// Since there is guaranteed to be at least one tab previous, this will never overflow
		q->quote[j] = '\0';
		
		// Finds the author of the quote
		for (i = line_num; i >= -1; i--)
		{
			// When no author could be found
			if (i == -1)
			{
				strcpy(q->author, "<UNKNOWN AUTHOR>");
				break;
			}
			
			// Tests if the beginning is the name of an author
			if (isAlphaNumeric(script[i][0]))
			{
				// Copies the name of the author into question
				for (j = 0; j < LINE_SIZE && script[i][j] != ':'; j++)
					q->author[j] = script[i][j];
				// Not guaranteed that this will not exceed the allocated size
				j == LINE_SIZE ? (q->author[LINE_SIZE-1] = '\0') : (q->author[j] = '\0');
				
				break;
			}
		}
	}
	// Quote is the first line by the author... (AUTHOR:\t..\tQUOTE)
	else
	{
		// Copies the name of the author into question
		for (i = 0; i < LINE_SIZE && script[line_num][i] != ':'; i++)
			q->author[i] = script[line_num][i];
		// Not guaranteed that this will not exceed the allocated size
		if (i == LINE_SIZE)
		{
			q->author[LINE_SIZE-1] = '\0';
			strcpy(q->quote, "<UNKNOWN QUOTE>");
		}
		else
		{
			q->author[i++] = '\0';
			
			// Go through all the whitespace
			while (i < LINE_SIZE && isWhiteSpace(script[line_num][i]))
				i++;
			
			// Copy the quote
			for (j = 0; i < LINE_SIZE && script[line_num][i] != '\0'; i++)
			{
				q->quote[j++] = script[line_num][i];
			}
			// Since there is guaranteed to be at least one ':' previous, this will never overflow
			q->quote[j] = '\0';
		}
	}
	
	// Gets the next author and next quote
	for (i = line_num+1; i <= lineCount; i++)
	{
		// Could not find a next quote or author, thus this is not a proper question
		if (i == lineCount)
		{
			free(q);
			return NULL;
		}
		
		// Invalid quote or author beginnings
		if (!isAlphaNumeric(script[i][0]) && (script[i][0] != '\t' || script[i][0] == ' '))
			continue;
		
		// Next quote from the same author
		if (script[i][0] == '\t' || script[i][0] == ' ')
		{
			// Sets the two authors to be the same
			strcpy(q->next_author, q->author);
			
			j = 0;
			// Creates the next quote by getting rid of all the previous whitespace
			while (j < LINE_SIZE && isWhiteSpace(script[i][j]))
				j++;
			for (; j < LINE_SIZE && script[i][j] != '\0'; j++)
				q->next_quote[k++] = script[i][j];
			// Since there is guaranteed to be at least one tab previous, this will never overflow
			q->next_quote[k] = '\0';
			
			break;
		}
		// Next quote from different author
		else
		{
			// Copies the name of the author into question
			for (j = 0; j < LINE_SIZE && script[i][j] != ':'; j++)
				q->next_author[j] = script[i][j];
			// Not guaranteed that this will not exceed the allocated size
			if (j == LINE_SIZE)
			{
				q->next_author[LINE_SIZE-1] = '\0';
				strcpy(q->next_quote, "<UNKNOWN QUOTE>");
			}
			else
			{
				q->next_author[j++] = '\0';
				
				// Go through all the whitespace
				while (j < LINE_SIZE && isWhiteSpace(script[i][j]))
					j++;
				
				// Copies the next quote
				for (k = 0; j < LINE_SIZE && script[i][j] != '\0'; j++)
				{
					if (script[i][j] == '\t')
						continue;
					q->next_quote[k++] = script[i][j];
				}
				// Since there is guaranteed to be at least one ':' previous, this will never overflow
				q->next_quote[k] = '\0';
			}
			
			break;
		}
	}
	
	// Removes trailing whitespace from the quotes
	removeTrailingWhitespace(q);
	
	return q;
}

// Finds a random quote in the script that has an author and a next quote
// If diffLineNum == -1, normal difficulty is active. If not, then this uses quotes within a radial distance of the line number passed
Question *findQuestion (char **script, int lineCount, int diffLineNum)
{
	Question *q;
	char *tmpstr;
	int i, line_num;
	
	if (script == NULL)
		return NULL;
	
	// Attempts to create questions from random lines of the script until it is a proper question or a set number of tries occur
	for (i = 0; q == NULL || i < ATTEMPTS; i++)
	{
		// Normal Game Difficulty or creation of original question (the correct answer)
		if (diffLineNum < 0 || diffLineNum >= lineCount)
			line_num = rand() % lineCount;
		// Hard Game Difficulty... Generates quotes within 10 spaces of the diffLineNum
		else
			line_num = ((rand() % 21) - 10) + diffLineNum;
		// Creates the question at line_num 
		q = createQuestion(script, lineCount, line_num);
	}
	
	// Returns the found quote
	return q;
}

// Returns whether q_choices[i]'s quote is a valid response
int isValidAnswerChoice (Question *q, Question **q_choices, int i)
{
	int j;
	
	if (q == NULL || q_choices == NULL || q_choices[i] == NULL)
		return 0;
	
	// Tests if the question and the answer aren't the same, unless if the next quote is the answer
	if (strcmp(q->quote, (q_choices[i])->next_quote) != 0 || strcmp(q->quote, q->next_quote) == 0 )
	{
		// Makes sure the answers are different from the correct answer
		if (strcmp(q->next_quote, (q_choices[i])->next_quote) != 0)
		{
			// Makes sure either the original author or the original quote are different
			if (strcmp(q->quote, (q_choices[i])->quote) != 0 || strcmp(q->author, (q_choices[i])->author) != 0)
			{
				// Checks to make sure no answer choices are repeated
				for (j = 0; j < i; j++)
				{
					if (strcmp((q_choices[j])->next_quote, (q_choices[i])->next_quote) != 0)
						;
					else
						return 0;
				}
				// If all conditions are passed, this is a proper answer choice
				return 1;
			}
		}
	}
	
	// Not a valid answer choice
	return 0;
}

// Will give a question to the player with given number of responses
int askQuestion (char **script, int lineCount, int question_num, int num_responses, int diffFlag)
{
	Question **q_choices;
	Question *q;
	char response;
	char tmp_response[LINE_SIZE], tmpstr[LINE_SIZE];
	time_t t;
	int i, j, correct_response;
	
	if (script == NULL)
		return 0;
	
	// Allocates memory for question choices and initializes them
	q_choices = malloc(num_responses * sizeof(Question*));
	if (q_choices == NULL)
		return 0;
	for (i = 0; i < num_responses; i++)
		q_choices[i] = NULL;
	
	// Seed the random number generator
	srand((unsigned) time(&t));
	
	q = findQuestion(script, lineCount, -1);
	if (q == NULL)
		err("Could not create a question for this script.");
	
	// Prints the question
	printf("\nQuestion #%d:\n", question_num+1);
	printf("After %s says\n\t\"%s\",\nwhat does %s say next?\n", q->author, q->quote, q->next_author);
	
	// Chooses a random response to be the correct one
	correct_response = rand() % num_responses;
	// Prints out the various answer choices
	for (i = 0; i < num_responses; i++)
	{
		// Prints the correct quote
		if (i == correct_response)
		{
			q_choices[i] = q;
			printf("%c. %s\n", 'A' + i, q->next_quote);
		}
		// Prints other randomly generated quotes
		else
		{
			// Finds a new question and checks the validity of the question
			for (j = 0; j < ATTEMPTS; j++)
			{
				// Finds a quote for the other answer choices, determines if difficulty plays a factor or not
				q_choices[i] = findQuestion(script, lineCount, diffFlag ? q->line_num : -1);
				
				// Checks if the generated question is a valid answer choice
				if (!isValidAnswerChoice(q, q_choices, i))
				{
					free(q_choices[i]);
					q_choices[i] = NULL;
				}
				else
					break;
			}
			
			// If a question could not be found
			if (q_choices[i] == NULL)
			{
				// If the correct response was already displayed, it will just disregard this and follwing questions
				if (correct_response < i)
					break;
				// Otherwise, the correct answer will be displayed now and then disregard all the following questions
				printf("%c. %s\n", 'A' + i, q->next_quote);
				correct_response = i;
				break;
			}
			
			// Prints the incorrect quotes
			printf("%c. %s\n", 'A' + i, (q_choices[i])->next_quote);
		}
	}
	
	// Gets user response
	printf("Answer: ");
	// Prevents users from causing a buffer overflow
	scanf("%"STRINGIFY(LINE_SIZE)"s", tmp_response);
	// Gets an answer from the user data
	response = cleanResponse(tmp_response);

	// Frees questions from memory	
	for (i = 0; i < num_responses; i++)
		if (q_choices[i] != NULL)
			free(q_choices[i]);
	free(q_choices);
	
	// Prints out result of question
	if (toLower(response) == 'a' + correct_response)
		printf("--CORRECT!--\n");
	else
		printf("--INCORRECT! The right choice was %c!--\n", 'A' + correct_response);
	
	// Returns whether the answer was right or not
	return toLower(response) == 'a' + correct_response;
}

void playQuiz (char **script, int lineCount, char *name, int num_questions, int num_responses, int diffFlag)
{
	Question *reward;
	int i, score = 0;
	
	if (script == NULL)
		return;
	
	// Opening statement
	printf("\nWelcome to Script Quiz!\n\n");
	printf("This is a %d question quiz where you will be given up\n", num_questions);
	printf("to %d possible response%sto a quote from the script:\n", num_responses, num_responses == 1 ? "" : "s ");
	printf("\t%s\n\n", name);
	printf("Type the corresponding letter to choose your answer. Good luck!\n\n");
	printf("Difficulty: %s\n\n\n", diffFlag ? "HARD" : "NORMAL");
	
	// Gives the player questions, if the player gets it right then their score increases
	for (i = 0; i < num_questions; i++)
		if (askQuestion(script, lineCount, i, num_responses, diffFlag))
			score++;
	
	// Shows their final score
	printf("\nYou got %d/%d correct.\n\n", score, num_questions);
	// Reward for scoring at least a 50%
	if (score * 1.0 / num_questions >= 0.5)
	{
		reward = findQuestion(script, lineCount, -1);
		printf("Congrats on getting %d correct!\n", score);
		printf("As a reward... %s has a special message for you...\n", reward->author);
		printf("\"%s\"\n\n", reward->quote);
		free(reward);
	}
	
	return;
}

// Format of Command Line Arguments:
// [Exceutable] my_script.txt NumberOfQuestions NumberOfResponses DifficultyFlag(0 for Normal, 1 for Hard)
// Default values are:
//   NumOfQuestions: 10
//   NumOfResponses: 5
//   Difficulty: NORMAL
int main (int argc, char **argv)
{
	char **script;
	int lineCount;
	char *strtol_pntr;
	// These are the default values for the game
	int num_questions = 10, num_responses = 5, diffFlag = 0;
	
	// Makes sure an argument is given
	if (argc < 2)
		err("No script given.");
	
	// Gets the amount of lines in the script
	lineCount = numOfLines(argv[1]);
	if (lineCount < 1)
		err("Could not read the script.");
	
	// Allocates memory for the script
	script = malloc(lineCount * sizeof(char *));
	if (script == NULL)
		err("Memory could not be allocated for the script.");
	
	compileScript(script, argv[1], lineCount);
	
	// Optional command line arguments for gameplay
	if (argc >= 3)
	{
		num_questions = (int)strtol(argv[2], &strtol_pntr, 10);		
		if (argc >= 4)
		{
			num_responses = (int)strtol(argv[3], &strtol_pntr, 10);
			if (argc >= 5)
				diffFlag = (int)strtol(argv[4], &strtol_pntr, 10);
		}
	}
	
	// Restriction on question count
	if (num_questions < 1)
		num_questions = 1;
	// Restriction on response count
	if (num_responses < 2)
		num_responses = 2;
	if (num_responses > 15)
		num_responses = 15;
	// Restriction on difficulty flag
	if (diffFlag < 0)
		diffFlag = 0;
	if (diffFlag > 1)
		diffFlag = 1;
	
	// Plays the game with chosen settings
	playQuiz(script, lineCount, argv[1], num_questions, num_responses, diffFlag);
	
	freeScript(script, lineCount);
	
	return 0;
}