/* 
 * File:        protictor.h
 * Author:      brishna
 * Description: header; defines globals and defaults for Protictor
 *
 * Created on August 31, 2018, 2:51 PM
 */

#ifndef PROTICTOR_H
#define PROTICTOR_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <errno.h>
#include <vector>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <map>
#include <algorithm>
#include <sstream>
#include <regex>


using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::ifstream;
using std::fstream;
using std::ofstream;
using std::endl;
using std::vector;
using std::map;
using std::regex;


//#define maxReadSize 1048576*1 // 1MB
#define oneMB 1048576 // 1MB
#define maxReadSize oneMB 
#define maxBufferSize oneMB*2 

struct TraceStep;

int readFromFIFO(int fifo , int outfifo , char buffer[]);
int writeToFIFO(int & fifo , string str);
int reopenFIFO(int & fifo , string fifoName);
int openWriteFIFO(string fifoName);
int openReadFIFO(string fifoName);
vector<string>  cleanChoicePrompt(const string longChoicePrompt);
int countOccurrences(string str , string substr);
vector<string> split(const string &s, char delim);
string ltrim(string str);
bool readPMLFile(string file);
string parseInteractiveSpinOutput(string text);
string cleanTraceFile();
bool parseTraceFile();
void printVector(vector<string> v);
void printVector(vector<string> v);
void printVector(vector<int> v);
void printVariableEvaluation(map<string,string> v);
void printRegexMatches(string text , regex pattern);
void printTraceStep(TraceStep ts);
void eprintTraceStep(TraceStep ts);
void eprintVariableEvaluation(map<string,string> v);
void fprintVector(vector<string> v , string filename);
void fprintVector(vector<TraceStep> v , string filename);
void performCorection();
void clearGlobals();
int determineScenario();
bool matchSteps(TraceStep step1 , TraceStep step2);
bool matchDSteps(TraceStep step1 , TraceStep step2);
bool checkIfErrorTraceWritten();

//int determineChoice(const string fifoName);
int findInCode(string str);
string getCodeAtLine(int l);
void setCodeAtLine(int l , string s);
string getCodeAtStepIndex(int l);
void setCodeAtStepIndex(int l , string s);
TraceStep getNextTraceStep(bool & hasEnded);
std::pair<string , string> extractVariable(string line);

// consolidating:
bool spinGivesError();
void setupCommunication();
void createFIFOs();

string traceFileName = "trace.txt";
string cleanedTraceFileName = "trace_clean.txt" , panOutput = "panOutput.txt";
//string infileName = "toy-maker.pml";
string infileName;
string infileName_original = infileName;
string childInFifoName = "childStdinFifo.txt";
string childOutFifoName = "childStdoutFifo.txt";
string spinPath = "/home/brishna/Software/Spin/Installation/tweaked";
vector<TraceStep> traceSteps;
vector<string> inputCode;
vector<string> correctredCode;
vector<int> stepsWithChoices;
int traceStepCounter = -1;
int numberOfIterations = 0;
int traceIterator = 0;
bool firstPromptCleaning = true;
bool feedbackStep = false;
bool errorExists = true;
bool correctionDiverges = false;
//TraceStep nextTraceStep;

struct TraceStep
{
    int stepNumber = -1;
    map<string,string> globalVariables;
    int processNumber = -1;
    string processName = "";
    int priority = -1;
    string filename = "";
    int lineNumber = -1;
    int stateNumber = -1;
    string instruction = "";
    
    void clear()
    {
        stepNumber = -1;
        processNumber = -1;
        processName = "";
        priority = -1;
        filename = "";
        lineNumber = -1;
        stateNumber = -1;
        instruction = "";  
        globalVariables.clear();
    }
    
    bool isEmpty()
    {
        return ((stepNumber == -1) && (processNumber == -1)); 
    }
//    choice #, proc #, proc name, proc priority, filename, line number, state #, code
};

TraceStep nextTraceStep;


#endif /* PROTICTOR_H */

