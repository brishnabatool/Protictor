/*
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

//#include <../5.4.0/bits/stl_vector.h>

#include "protictor.h"
int main(int argc, char **argv)
{                                                                                                                                                   
    
    if(argc > 1)
        infileName = argv[1];
    else
    {
        std::cerr << "\n\nUsage: protictor <file>\n\n";
        return(1);
    }

    createFIFOs();
                                                        
    pid_t pid;
    int returnCode = 0 , childWait = 0;
    string commandstr;
    
    
    numberOfIterations = 0;
//    while(numberOfIterations<1 && errorExists && !correctionDiverges)
    while(errorExists && !correctionDiverges)
    {
        cout << "\n\n!!!!!!!!\nStarting process for file " << infileName << "\n\n";

        // run spin to generate verifier
        //execute the command in a forked child
        commandstr = spinPath + "/spin -a " + infileName.c_str() ;  // run tweaked SPIN
        returnCode = system(commandstr.c_str());
        if(returnCode < 0)
        {                
            cerr<<"\nERROR! Verifier could not be generated.\n"; //only executed when execlp fails
            exit(1);
        }
        cout<<"\nVerifier generated.\n";


        // compile the verifier        
        commandstr = "gcc -DMEMLIM=1024 -O2 -DXUSAFE -DNOREDUCE -w -o pan pan.c";
        returnCode = system(commandstr.c_str());
        if(returnCode < 0)
        {                
            cerr<<"\nERROR! Verifier could not be compiled.\n"; //only executed when execlp fails
            exit(1);
        }
        cout<<"\nVerifier compiled.\n";

        // execute the verifier
        commandstr = "./pan -m10000 -a > " + panOutput ;  // run tweaked SPIN
        returnCode = system(commandstr.c_str());
        if(returnCode < 0)
        {                
            cerr<<"\nERROR! Verifier could not be executed.\n"; //only executed when execlp fails
            exit(1);
        }
        cout<<"\nVerifier executed.\n";
        
        // check if error trace written
        errorExists = checkIfErrorTraceWritten();
        if(!errorExists)
        {
            break;
        }

        // simulate the error trace and write output to traceFIleName (trace.txt)
        commandstr = spinPath + "/spin -p -v -s -r -X -n123 -g -w -k " + infileName + ".trail" + " -u10000 " + infileName + " > " + traceFileName ;  // run tweaked SPIN
        returnCode = system(commandstr.c_str());
        if(returnCode < 0)
        {                
            cerr<<"\nERROR! Trail file could not be simulated.\n"; //only executed when execlp fails
            exit(1);
        }
        cout << "\nCounter-example trace written to " << traceFileName;
        
        readPMLFile(infileName);
        parseTraceFile();

        // create new process to execute SPIN in interactive mode
        pid = fork();

        if(pid == 0)    // child
        {
            cerr << "\nChild starting\n";
            int execReturn = -70 , dupReturn = -70 ;

            // redirect stdin to named pipe
            // first, open pipe in read mode
            int childFin = open(childInFifoName.c_str() , O_RDONLY);
            if(childFin < 0)
            {
                cerr << "\nUnable to open fifo for reading stdin. " << childInFifoName << "\n";
                cerr << strerror(errno);
                exit(errno);
            }

            // then, redirect stdin to pipe
            // if (dup2(childStdinPipe[PIPE_READ], 0) == -1) 
            dupReturn = dup2(childFin, STDIN_FILENO);
            if (dupReturn == -1) 
            {
                cerr << "\ndup2 returned " << dupReturn ;
                cerr << "\nUnable to redirect stdin to pipe.\n";
                cerr << strerror(errno);

                close(childFin);
                exit(errno);
            }
            else
            {
                cerr << "\nRedirected stdin to fifo " << childFin;
                cerr << "\ndup2 returned " << dupReturn;
            }

            // redirect stdout to named pipe
            // first, open pipe in read mode
            int childFout = open(childOutFifoName.c_str() , O_WRONLY|O_SYNC);
            if(childFout < 0)
            {
                cerr << "\nUnable to open fifo for writing stdout. " << childOutFifoName << "\n";
                cerr << strerror(errno);
                close(childFout);
                exit(errno);
            }

            // then, redirect stdout to pipe
            dupReturn = dup2(childFout, STDOUT_FILENO);
            if (dupReturn == -1) 
            {
                cerr << "\ndup2 returned " << dupReturn ;
                cerr << "\nUnable to redirect stdout to pipe.\n";
                cerr << strerror(errno);

                close(childFin);
                close(childFout);
                exit(errno);
            }
            else
            {
                cerr << "\nRedirected stdout to fifo " << childFout;
                cerr << "\ndup2 returned " << dupReturn;
            }

            std::cerr << "\nAbout to launch";

            execReturn = execlp((spinPath + "/spin") .c_str() , "spin" , "-i" , "-p" , "-w" , "-g" , infileName.c_str() , NULL);// run tweaked SPIN
    //        execReturn = execlp("spin" , "spin" , "-i" , "-p" , "-w" , "-g" , infileName.c_str() , NULL);// run tweaked SPIN

            // if we get here at all, an error occurred, but we are in the child
            // process, so just exit

            std::cerr << "Failed to exec. ";
            cerr << strerror(errno);

            // write(outFile , s.c_str() , s.length()+1);
            close(childFin);     // close outFile
            close(childFout);     // close outFile 

            exit(execReturn);
        }

        else if(pid > 0)   // parent
        {
            std::cout.setf(std::ios::unitbuf);
            // parent
            cout << "\nParent starting\n";
            
            if(numberOfIterations > 0)
            {
                cout << "\npause here";
            }   
             

            ssize_t bytesRead;
            int inFile;
//            char buffer[maxReadSize];
            char buffer[maxBufferSize];
            string str = "empty";
            int spinChoice = 1;

            // open fifo for output
            // int parentFout = open(childInFifoName.c_str() , O_WRONLY|O_SYNC);
            int parentFout = open(childInFifoName.c_str() , O_RDWR|O_SYNC);
            if(parentFout < 0)
            {
                cerr << "\nUnable to open stdin fifo for writing. " << childInFifoName;
                cerr << strerror(errno);
                exit(errno);
            }

            // open fifo for input
            int parentFin = open(childOutFifoName.c_str() , O_RDONLY);
            if(parentFin < 0)
            {
                cerr << "\nUnable to open stdout fifo for reading. " << childOutFifoName;
                cerr << strerror(errno);
                exit(errno);
            }

            string choice = "";
            int count = 0;

            // start communicating with child
            // according to sequence diagram

            bool quitSpin = false;
            int numberOfSpinIterations = 0;

            // read child's first message
            readFromFIFO(parentFin , parentFout , buffer);
            choice = parseInteractiveSpinOutput(buffer);

            while(choice != "q")
            {
                numberOfSpinIterations ++;
                // cout << "\nnumberOfSpinIterations = " << numberOfSpinIterations << endl;

                writeToFIFO(parentFout , choice);
                // reopenFIFO(parentFout , childInFifoName);

                readFromFIFO(parentFin , parentFout , buffer);

                // make decision of choice by analyzing options provided and error trace
                choice = parseInteractiveSpinOutput(buffer);
            }

            // terminate SPIN -i
            cout << "\n\nExiting loop. Killing child.";
            kill(pid , SIGKILL);
            close(parentFin);
            close(parentFout);
            cout << "\nThe child has been terminated." << endl;
            
//            cout << "\nExiting loop. terminating child" << endl;
//            writeToFIFO(parentFout , "q");              // send quit signal to child
//           
//            cout << "\nWaiting for child to terminate." << endl;
//            int status;
//            wait(&status);
//            cout << "\nThe child has terminated with status " << status << endl;

            // perform correction
            performCorection();
        }

        else    // failed to fork
        {
            // close(childStdinPipe[PIPE_READ]);
            // close(childStdinPipe[PIPE_WRITE]);
        }
    
        // clear vectors for next round
        clearGlobals();

        numberOfIterations++;
        
        cout << "\n\nITERATION " << numberOfIterations << " complete";
        cout << "\nNew file is " << infileName << "\n";
        
        if(numberOfIterations > 255)
        {
            correctionDiverges = true;
        }
    }
    
    if(!errorExists)
    {
        cout << "\n\nThe model has been corrected in " << numberOfIterations << " iterations.";
        cout << "\nThe corrected model has been written to the file " << infileName << "\n";
    }

    if(numberOfIterations > 255)
    {
        correctionDiverges = true;
        cout << "\nAutomated correction unsuccessful. Corrections diverge. Aborting.";
    }
    
    return 0;
}


int readFromFIFO(int fifo , int outfifo , char buffer[])
{
    int bytesRead = -1;
    int rcount = 0;
    bool allRead = false , sizeLimitReached = false;

    cout << "\n\nReading from child";
    
    buffer[0] = '\0';    
    while(!allRead && !sizeLimitReached)
    {
        char tmpbuf[maxReadSize];
//        tmpbuf[0] = '\0';    
        bytesRead = -1;
        bytesRead = read(fifo , tmpbuf , maxReadSize-1);
        if (bytesRead <= 0)    
        {
            printf("\tno input\n");
            printf("\tbuffer so far:\n%s" , buffer);
       
            cerr << "\nAborting. No input received from SPIN.\n";
            exit(1);
        }
//        else
        {   
            tmpbuf[bytesRead] = '\0';
    //        cout << "\nthe child's message is:\n" << buffer << " in " << bytesRead << " bytes.";
            cout << "\nthe child's message is:" << bytesRead << " bytes long.";
            if(bytesRead == 639)
            {
                cout << "\npause here";
            }
//            cout << "\nthe child's message is:" << tmpbuf << "\n\n";
            if((strlen(buffer) + strlen(tmpbuf)) >= maxBufferSize - 1)
            {
                cout << "\nInteractive output exceeds maximum buffer size. Cycle likely running.";
                sizeLimitReached = true;
            }
            else
            {                
                strcat(buffer , tmpbuf);
                if(strstr(buffer , "###allsent###"))
                {
                    // make sure all tags have been sent and received
                    allRead = true;
                    if(countOccurrences(buffer , "###end###") < rcount)
                    {
                        cout << "\nGetting remaining end tags";
                        allRead = false;
                        
//                        writeToFIFO(outfifo , "r");
//                        rcount++;
                    }
//                    cout << "\nGot all " << rcount << " end tags";
                }
                else
                {
                    writeToFIFO(outfifo , "r");
                    rcount++;
                }
            }
        }
    }
    // remove marker from buffer
    cout << "\n\nRead complete.";

    if(!sizeLimitReached)
    {        
        char * end = strstr(buffer , "###allsent###");
        while(end == buffer)    // if old tag received in new read
        {
            buffer = buffer + 13;
            end = strstr(buffer , "###allsent###");
        }
        *end = '\0';
    }
    
    cout << "\nthe child's message is:\n" << buffer;
    
    
    // cout << "\nRead from child";
    cout.flush();
}

int writeToFIFO(int & fifo , string str)
{
    int bytesWritten = -1;
    // str += "\r";

    char buffer [2];
    buffer[0] = str[0];
    buffer[1] = '\r';
    cout << "\nWriting to child: "<< buffer << "\n";

    bytesWritten = write(fifo, buffer, 2);

    cout << "\nnumber of bytes written: "<< bytesWritten;
    cout.flush();

    return bytesWritten;
}

int reopenFIFO(int & fifo , string fifoName)
{
    cout << "\n\nreopening stdin fifo";
    
    close(fifo);

    // fifo = open(fifoName.c_str() , O_WRONLY|O_SYNC|O_TRUNC);
    // if(fifo < 0)
    // {
    //     cerr << "\nUnable to open fifo for writing " << fifoName;
    //     cerr << strerror(errno);
    //     exit(errno);
    // }
    // cout << "\nreopened stdin fifo";
    // cout.flush();
    // fflush
    
    return fifo;
}

int openWriteFIFO(string fifoName)
{
    int fifo;
    cout << "\n\nOpening fifo " << fifoName;
    
    fifo = open(fifoName.c_str() , O_WRONLY|O_SYNC);
    
    if(fifo < 0)
    {
        cerr << "\nUnable to open fifo for writing " << fifoName;
        cerr << strerror(errno);
        exit(errno);
    }
    // cout << "\nOpened fifo" << fifoName << "for writing";
    cout.flush();

    return fifo;
}

int openReadFIFO(string fifoName)
{
    int fifo;
    cout << "\n\nOpening fifo " << fifoName;
    
    fifo = open(fifoName.c_str() , O_RDONLY);
    
    if(fifo < 0)
    {
        cerr << "\nUnable to open fifo for writing " << fifoName;
        cerr << strerror(errno);
        exit(errno);
    }
    // cout << "\nOpened fifo" << fifoName << "for reading";
    cout.flush();

    return fifo;
}

int countOccurrences(string str , string substr)
{       
    int count = 0;
    std::string::size_type start_pos = 0;
    while( std::string::npos != (start_pos = str.find(substr, start_pos) ))
    {
        // do something with start_pos or store it in a container
        ++start_pos;
        count ++;
    }

    return count;
}

vector<string> split(const string &s, char delim) 
{
    std::stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        item = ltrim(item);
        // tokens.push_back(".."+item+"..");
        tokens.push_back(item);
    }

    return tokens;
}

string ltrim(string str)
{

            // cout << "\nxxxxxxxxx " << str;
    bool found = false;
    int i = 0;
    while(!found && i<str.length())
    {
        if((str[i] == ' ' || str[i] == '\t'));
        else
        {
            // cout << "\nzzzzzzzzzz " << i;
            found = true;
            i--;
        }
        i++;
    }

    return str.substr(i);

}

void printVector(vector<string> v)
{
    for (vector<string>::iterator i = v.begin(); i != v.end(); ++i)
        std::cout << "\n.. " << *i << " ..";
}


void printVector(vector<int> v)
{
    for (vector<int>::iterator i = v.begin(); i != v.end(); ++i)
        std::cout << "\n" << *i;
}

void printVector(vector<map<string,string>> v)
{
    for (auto i : v ) 
    {
        cout << "\n";
        printVariableEvaluation(i);     
    }
}

void printVariableEvaluation(map<string,string> v)
{
    for(auto elem : v)
    {
       cout << "\n" << elem.first << " <--- " << elem.second;
    }

}

void eprintVariableEvaluation(map<string,string> v)
{
    for(auto elem : v)
    {
       cerr << "\n" << elem.first << " <--- " << elem.second;
    }

}

void fprintVector(vector<string> v , string filename)
{
    std::ofstream ofile;
    ofile.open (filename);

    for (vector<string>::iterator i = v.begin(); i != v.end(); ++i)
        ofile << *i << "\n";

    ofile.close();
}

void fprintVector(vector<TraceStep> v , string filename)
{
    std::ofstream ofile;
    ofile.open (filename);
    
//    ofile << v.size() << "\n\n";
    for (vector<TraceStep>::iterator i = v.begin(); i != v.end(); ++i)
    {
        ofile << i->stepNumber;
        ofile << ":\tproc ";
        ofile << i->processNumber;
        ofile << "\t(";
        ofile << i->processName;
        ofile << ":";
        ofile << i->priority;
        ofile << ")\t";
        ofile << i->filename;    
        ofile << ":";
        ofile << i->lineNumber;
        ofile << "\t(state ";
        ofile << i->stateNumber;
        ofile << ")\t[";
        ofile << i->instruction;
        ofile << "]";
        for(auto elem : i->globalVariables)
        {
            ofile << "\n" << elem.first << " <--- " << elem.second;
        }
        ofile << "\n\n";
    }

    ofile.close();
}

bool readPMLFile(string file)
{
    std::ifstream infile(file.c_str());

    string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        // cout << "\nline: " << line;
        inputCode.push_back(line);
    }

    return false;   
}

// remove NBA info from trace file and perform other minor formatting changes
string cleanInteractivePropmpt(string prompt)
{
    if(numberOfIterations > 0)
    {
        
        cout << "\npause here";
    }
    cout << "\nCleaning prompt";

    string line;
    if(firstPromptCleaning)
        prompt = "##\n" + prompt;  // appended to start of trace file to facilitate removal of preamble
    
//    // fourth pass: remove "MSC: ~G line..."
//    pattern = ".*MSC: ~G line.*\\n";
//    completeTrace = std::regex_replace (completeTrace,pattern,"");
//    
    // first pass: remove extra info at start of first prompt
    regex pattern ("##(.*\\n*)*?(.*proc\\s*0\\s*\\(:init::)");
    if(firstPromptCleaning)
        prompt = std::regex_replace (prompt,pattern,"$2");

    // second pass: remove "Starting process..." for run() statements
    pattern = "Starting .* with pid.*";
    prompt = std::regex_replace (prompt,pattern,"");

    // third pass: remove "...creates process..." for first run() statement
    pattern = "^.*creates proc.*\n";
    prompt = std::regex_replace (prompt,pattern,"\n");
   
    // fourth pass: remove "...creates process..." for subsequent run() statements
    pattern = "\n.*creates proc.*\n";
    prompt = std::regex_replace (prompt,pattern,"\n");

//    // fifth pass: reformat "...other process..." option as valid step with dummy values for other fields
//    pattern = ".*other process.*\n";
//    prompt = std::regex_replace (prompt,pattern,"");
//    
        
    // fifth pass: remove "goto :bn" steps
    pattern = ".*goto \\:b\\d+.*([\\n\\s\\w\\d=-_\\(\\)\\[\\]\\{\\}\\.])*\n\n";
    prompt = std::regex_replace (prompt,pattern,"\n");

    // sixth pass: ensure all prompts are followed by "Select [x-y]:
    pattern = "(choice (\\d*):\\s*proc\\s*\\d+\\s*\\(.+\\:\\d+\\)\\s*.+\\:\\d+\\s*\\(state\\s*\\d+\\)\\s*\\[.+\\])\n(\\s*\\d+\\:)";
    prompt = std::regex_replace (prompt,pattern,"$1\nSelect [0-0]: $2\n$3");
    
//    ^(\s*choice \d*:\s*proc\s*\d+\s*\(.+\:\d+\)\s*.+\:\d+\s*\(state\s*\d+\)\s*\[.+\])\n(\s*\d+\:)
         
    // seventh pass: remove "<valid end state>"
    pattern = "<\\s*valid end state\\s*>";
    prompt = std::regex_replace (prompt,pattern,"");
    
//    // eighth pass: remove statement merging info
//    pattern = "<\\s*merge.*>";
//    completeTrace = std::regex_replace (completeTrace,pattern,"");
//    
    firstPromptCleaning = false;
    cout << "\nCleaned prompt";
    return prompt;
}

// parse the output of spin -i
string parseInteractiveSpinOutput(string prompt)
{            
    cout << "\nParsing Output";
//    cout << "\n\nPROMPT:\n" << prompt << "\n\n";
    
    string choice;
    string line = "";
    int numberOfChoices = 0;
    bool endOfPrompt = false;
    bool endOfTrace = false;
    bool retry = false;
    TraceStep interactiveStep;
    regex basicPattern ("\\s*(\\d+):\\s*proc\\s*(\\d+)\\s*\\((.+)\\:(\\d+)\\)\\s*(.+)\\:(\\d+)\\s*\\(state\\s*(\\d+)\\)\\s*\\[(.+)\\]");
    regex terminationPattern ("\\s*(\\d+):\\s*proc\\s*(\\d+).*terminates");
    regex choicePattern ("\\s*choice (\\d*):\\s*proc\\s*(\\d+)\\s*\\((.+)\\:(\\d+)\\)\\s*(.+)\\:(\\d+)\\s*\\(state\\s*(\\d+)\\)\\s*\\[(.+)\\](:\\n\\s*\\:\\:\\s*.*)*");
    regex subChoicePattern ("^\\s*\\:\\:\\s*(.+)\\:(\\d+)\\s*\\[(.+)\\]");
    regex variableEvaluationPattern ("^\\s*([\\d\\a-zA-Z\\[\\]\\-\\_]*)\\s*\\=\\s*([\\d\\a-zA-Z\\-\\_]*)");
    regex tempPattern;
    std::smatch subCapturedGroups;
   
    if(firstPromptCleaning)     // get the first trace step
        nextTraceStep = getNextTraceStep(endOfTrace);
    prompt = cleanInteractivePropmpt(prompt);
    cout << "\n\nCLEANED PROMPT:\n" << prompt << "\n\n";
    
    prompt += " #end#";
    
    if(numberOfIterations > 0)
    {
        cout << "\npause here";
    }

    // iterate over text and identify and store each step with all its info
    while(!endOfPrompt) 
    {
        // clean interactive step for new iteration
        if(!retry)
        {
            interactiveStep.clear();            
        }
        else
        {
            retry = false;
        }
        cout << "\n$";
          
        // return 'q' if trace has ended
        if(endOfTrace)
        {
            cout << "\nEnd of trace reached.";
            return "q";
        }
        // abort if step is invalid
        if(nextTraceStep.isEmpty())
        {
            cerr << "\nAborting: Encountered invalid step in trace at index " << traceStepCounter;
            cerr << "\nLast step successfully read:\n";
            printTraceStep(traceSteps.at(traceStepCounter-1));
            exit(1);
        }
   
        // get interactive step
        //      i reported last step
        //      ii prompt for choice
        //          a. choice automatically taken
        //          b. real prompt for choice
        
        
        // check if skippable option (eg .goto?)
        // check by elimination for: break
        
        // try for reported step
        
//        // get next line
//        line = prompt.substr(0 , prompt.find('\n'));
//        
        line = prompt.substr(0 , prompt.find('\n'));
        // skip empty lines
        while (line == "")
        {
            prompt = prompt.substr(1); // skip line
            line = prompt.substr(0 , prompt.find('\n'));
        }
        
        if (line.find("#end#") != string::npos)
        {
            endOfPrompt = true;
            break;
        }

        std::smatch capturedGroups; // TO DO : move this outside the loop. Right now, causes Exception when moved up in debug mode (Cannot access memory address 0x0)
            
        // if reported step is feedback of choice, skip it.
        if(feedbackStep)
        {
            cout << "\nFeedback step present";
            
            // skip if termination
            if(regex_search(line, capturedGroups, terminationPattern))
            {
                cout << "\nFeedback step (termination)";
                
                prompt = prompt.substr(line.length()+1); // skip the whole match
                line = prompt.substr(0 , prompt.find('\n'));
            }
             
            // skip if regular step
            else if(regex_search(line, capturedGroups, basicPattern))
            {
                cout << "\nFeedback step (normal)";
                
                // skip variables
                prompt = prompt.substr(line.length()+1); // skip the whole match
                line = prompt.substr(0 , prompt.find('\n'));

                while(regex_search(line, capturedGroups, variableEvaluationPattern))
                {
                    prompt = prompt.substr(line.length()+1); // skip the whole match
                    line = prompt.substr(0 , prompt.find('\n'));
                }
            }
            
            // report if step unidentified
            else
            {
                cout << "\nFeedback step (undetermined)";
            }
            
            feedbackStep = false;
        }

        //      check for termination
        else if(regex_search(line, capturedGroups, terminationPattern))
        {
            cout << "\nTermination found\n";
            
                      
            // store captured fields
            interactiveStep.stepNumber = stoi(capturedGroups[1].str());
            interactiveStep.processNumber = stoi(capturedGroups[2].str());
            interactiveStep.instruction = "-end-";  
            

            if(matchSteps(interactiveStep , nextTraceStep))
            {
                cout << "\nProcess termination match found";

                nextTraceStep = getNextTraceStep(endOfTrace);
                prompt = prompt.substr(line.length()+1); // skip the whole match
                line = prompt.substr(0 , prompt.find('\n'));
            }               
        }
        // check for normal step
        else if(regex_search(line, capturedGroups, basicPattern))
        {
            cout << "\nNormal step found\n";
            
            // store captured fields
            interactiveStep.stepNumber = stoi(capturedGroups[1].str());
            interactiveStep.processNumber = stoi(capturedGroups[2].str());
            interactiveStep.processName = capturedGroups[3].str();
            interactiveStep.priority = stoi(capturedGroups[4].str());
            interactiveStep.filename = capturedGroups[5].str();    
            interactiveStep.lineNumber = stoi(capturedGroups[6].str());
            interactiveStep.stateNumber = stoi(capturedGroups[7].str());
            interactiveStep.instruction = capturedGroups[8].str();

            if(interactiveStep.instruction.find("goto") != string::npos)
            {
                cout << "\ngoto exists";
            }
            // skip variables
            prompt = prompt.substr(line.length()+1); // skip the whole match
            line = prompt.substr(0 , prompt.find('\n'));
            while(regex_search(line, capturedGroups, variableEvaluationPattern))
            {
                // get next line
                prompt = prompt.substr(line.length()+1); // skip the whole match
                line = prompt.substr(0 , prompt.find('\n'));
            }         
            
            // Try to match step with trace step
            if(matchSteps(interactiveStep , nextTraceStep))
            {
                cout << "\nStep matched";
                nextTraceStep = getNextTraceStep(endOfTrace);
            }                           
            
            else
            {
                cout << "\nComplicated case found";
                
                regex skipPattern ("goto\\s+(?!:)");
                std::smatch capturedGroups;
                                
                // .goto
                if((interactiveStep.instruction).find(".(goto)") != string::npos)
                {
                    cout << "\nSkipping .goto found in interactive mode";
                    // skip 
                }
                // goto only in interactive, not in trace
                else if(regex_search(interactiveStep.instruction, capturedGroups, skipPattern))
                {
                    cout << "\nSkipping goto found only in interactive mode";
                    // skip 
                }
//                // d_step
//                else if((getCodeAtLine(nextTraceStep.lineNumber)).find("d_step") != string::npos)  
//                {
//                    if(matchDSteps(interactiveStep , nextTraceStep))
//                    {
//                        // correct line number in trace
//                        traceSteps.at(traceStepCounter).lineNumber = interactiveStep.lineNumber;
//                        nextTraceStep.lineNumber = interactiveStep.lineNumber;
//                        cout << "\nModifying trace step line number for d_step";
//                        retry = true;
//                    }
//                }
            }
                 
            // step read completely           
        }
        
        // check for options (statement)
//        else if(line.find("Select a statement") != string::npos)
        else if(line.find("Select a statement") != string::npos || (line.find("Select stmnt") != string::npos))
        {
            // do something
            // get choices
            prompt = prompt.substr(line.length()+1); // skip the whole match
            line = prompt.substr(0 , prompt.find('\n')); // get next line
             
            string options = "";
            string optionsIterator = "";
            while(regex_search(line, capturedGroups, choicePattern))
            {
                // choice #, proc #, proc name, proc priority, filename, line number, state #, code
//                interactiveStep.processNumber
                
                options += line;
                prompt = prompt.substr(line.length()+1); // skip the whole match
                line = prompt.substr(0 , prompt.find('\n')); // get next line  
                
                // check if has sub options
                while(regex_search(line, subCapturedGroups, subChoicePattern))
                {
                    options += line;
                    prompt = prompt.substr(line.length()+1); // skip the whole match
                    line = prompt.substr(0 , prompt.find('\n')); // get next line  

                    prompt = prompt.substr(line.length()+1); // skip the whole match
                    line = prompt.substr(0 , prompt.find('\n')); // get next line  
                }
                options += "%%%";
            }

            
            // if choice already taken, lookup and match selected choice with trace
            tempPattern = "\\s*Select\\s*\\[\\d+\\s*\\-\\s*\\d+\\]\\s*\\:\\s*(\\d+)";
            std::smatch optionMatch;
            if(regex_search(line, capturedGroups, tempPattern))
            {
                 // compare options one by one
                optionsIterator = options.substr(0 , options.find("%%%")); // get next option
                options = options.substr(optionsIterator.length()+3); // skip the whole match
                    
                while(regex_search(optionsIterator, optionMatch, choicePattern))
                {
//                    cout << "\nNormal step found\n";

                    if(stoi(capturedGroups[1].str()) == stoi(optionMatch[1].str()))
                    {
                        // choice #, proc #, proc name, proc priority, filename, line number, state #, code

                        // store captured fields
                        interactiveStep.stepNumber = stoi(optionMatch[1].str());
                        interactiveStep.processNumber = stoi(optionMatch[2].str());
                        interactiveStep.processName = optionMatch[3].str();
                        interactiveStep.priority = stoi(optionMatch[4].str());
                        interactiveStep.filename = optionMatch[5].str();    
                        interactiveStep.lineNumber = stoi(optionMatch[6].str());
                        interactiveStep.stateNumber = stoi(optionMatch[7].str());
                        interactiveStep.instruction = optionMatch[8].str();
                        
                        if(matchSteps(interactiveStep , nextTraceStep))
                        {
                            cout << "\nMatch found";
                            nextTraceStep = getNextTraceStep(endOfTrace);
                            feedbackStep = true;
                        }
                        
                        else while(regex_search(optionsIterator, subCapturedGroups, subChoicePattern))
                        {                          
                            interactiveStep.lineNumber = stoi(subCapturedGroups[2].str());
                            interactiveStep.instruction = subCapturedGroups[3].str();
                            if(matchSteps(interactiveStep , nextTraceStep))
                            {
                                cout << "\nMatch found";
                                nextTraceStep = getNextTraceStep(endOfTrace);
                                feedbackStep = true;
                            }
                            else
                            {
                                cout << "\nStill looking within sub choices";
                            }
                        }
                    
                        prompt = prompt.substr(line.length()+1); // skip the whole match
                        line = prompt.substr(0 , prompt.find('\n')); // get next line                
                        break;
                    }
                    
                    cout << "\nGoing to try next option";
                    if(options.length() >= 3 + (options.substr(0 , options.find("%%%"))).length())
                    {
                        optionsIterator = options.substr(0 , options.find("%%%")); // get next option
                        options = options.substr(optionsIterator.length()+3); // skip the whole match
                    }
                    else
                    {
                        cout << "\nNo available options match trace";
                        cout << "\nTrace Step:\n";
                        printTraceStep(nextTraceStep);
                        cout << "\n\n\nAborting.";
                        exit(1);
                    }
                }                 
            }
            
            // if true choice prompt
            else
            {
                tempPattern = "\\s*Select\\s\\[\\d+\\s*-\\s*\\d+\\]\\s*\\:\\s*#end#";
                if(regex_search(line, capturedGroups, tempPattern))
                {
                    // compare options one by one
//                    options = options.substr(3); // skip the whole match
                    if(numberOfIterations > 0)
                    {
                        cout << "\npause here";
                    }
                    numberOfChoices = countOccurrences(options , "%%%");
                    optionsIterator = options.substr(0 , options.find("%%%")); // get next option
                    options = options.substr(optionsIterator.length()+3); // skip the whole match
                    while(regex_search(optionsIterator, optionMatch, choicePattern))
                    {
//                        numberOfChoices ++;
                        // choice #, proc #, proc name, proc priority, filename, line number, state #, code

                        // store captured fields
                        interactiveStep.stepNumber = stoi(optionMatch[1].str());
                        interactiveStep.processNumber = stoi(optionMatch[2].str());
                        interactiveStep.processName = optionMatch[3].str();
                        interactiveStep.priority = stoi(optionMatch[4].str());
                        interactiveStep.filename = optionMatch[5].str();    
                        interactiveStep.lineNumber = stoi(optionMatch[6].str());
                        interactiveStep.stateNumber = stoi(optionMatch[7].str());
                        interactiveStep.instruction = optionMatch[8].str();

                        if(matchSteps(interactiveStep , nextTraceStep))
                        {
                            nextTraceStep = getNextTraceStep(endOfTrace);
                            choice = optionMatch[1].str();
                            cout << "\nMatch found";
                            cout << "\nChoice should be: " << choice;
                            feedbackStep = true;    // next reported step will be a feedback step
                            // add choice to list of steps with nondeterministic choices
                            if(numberOfChoices > 1 && interactiveStep.instruction.find("-end-") == string::npos && interactiveStep.instruction.find("terminates") == string::npos)
                                stepsWithChoices.push_back(traceStepCounter-1);
                            
                            if(endOfTrace)
                            {
                                choice = "q";
                            }
                           
                            return choice;
                        }
                        else
                        {
                            cout << "\nStill looking";
                        }
                        if(options.length() >= 3 + (options.substr(0 , options.find("%%%"))).length())
                        {
                            optionsIterator = options.substr(0 , options.find("%%%")); // get next option
                            options = options.substr(optionsIterator.length()+3); // skip the whole match
                        }
                        else
                        {
                            if(numberOfChoices == 1)
                            {
                                
                                choice = optionMatch[1].str();
                                return choice;
//                                choice = optionsIterator[8];
//                                choice = "0";
                            }
                            cout << "\nNo available options match trace";
                            cout << "\nTrace Step:\n";
                            printTraceStep(nextTraceStep);
                            cout << "\n\n\nAborting.";
                            exit(1);
                        }
                    }
                }
                
                else
                {
                    cout << "\n problem in true prompt regex match";
                }
            }
            // otherwise, if really asking for choice, iterate over each choice one by one and match.
        }

//        // check for options (stmnt)
//        else if(line.find("Select stmnt") != string::npos)
//        {
//            // do something
//        }
        
        else
        {
            cerr << "\nEncountered unknown pattern in interactive prompt:\n" << line;
            exit(1);
        }

//        // advance prompt
//        prompt = prompt.substr(line.length()+1); // skip the whole match
    } 
    cout << "\nParsed output";
    
cout << "\nCound not determine. Enter choice or 'q' to exit > ";
cin >> choice;
 
    // add choice to list of steps with nondeterministic choices
    if(choice != "q")
    {
        stepsWithChoices.push_back(traceStepCounter-1);
    }
    return choice;
}

// remove NBA info from trace file and perform other minor formatting changes
string cleanTraceFile()
{
    cout << "\nCleaning trace";

    std::ofstream ofile;
    ofile.open (cleanedTraceFileName);
    std::ifstream infile(traceFileName.c_str());
    
    string line;
    string completeTrace = "##\n";  // appended to start of trace file to facilitate removal of preamble

    // read trace file
    while (std::getline(infile, line))
    {
        completeTrace += line += "\n";
    }
    
    // first pass: remove "Never claim moves..."
    regex pattern ("\\nNever claim moves.*\\n");
    completeTrace = std::regex_replace (completeTrace,pattern,"\n");
    
    // second pass: remove NBA steps
    pattern = ".*nvr.*([\\n\\s\\w\\d=-_\\(\\)\\[\\]\\{\\}\\.])*\n\n";
    completeTrace = std::regex_replace (completeTrace,pattern,"\n");
    
    // third pass: remove "START OF CYCLE"
    pattern = ".*START OF CYCLE.*\\n";
    completeTrace = std::regex_replace (completeTrace,pattern,"");

    // fourth pass: remove "MSC: ~G line..."
    pattern = ".*MSC: ~G line.*\\n";
    completeTrace = std::regex_replace (completeTrace,pattern,"");
    
    // fifth pass: remove extra info at start of file
    pattern = "##(.*\\n*)*?(.*proc\\s*0\\s*\\(:init:)";
    completeTrace = std::regex_replace (completeTrace,pattern,"$2");
        
    // sixth pass: remove extra info at end of file
    pattern = "spin:.*(\\n.*)*Exit-Status.*";
    completeTrace = std::regex_replace (completeTrace,pattern,"");
    
    // seventh pass: remove "Starting process..." for run() statements
    pattern = "Starting .* with pid.*";
    completeTrace = std::regex_replace (completeTrace,pattern,"");
        
    // eighth pass: remove statement merging info
    pattern = "<\\s*merge.*>";
    completeTrace = std::regex_replace (completeTrace,pattern,"");
    
    // ninth pass: remove "goto :bn" steps
    pattern = ".*goto \\:b\\d+.*([\\n\\s\\w\\d=-_\\(\\)\\[\\]\\{\\}\\.])*\n\n";
    completeTrace = std::regex_replace (completeTrace,pattern,"\n");
    
    // print the cleaned trace in the new file
    ofile << completeTrace << "\n";          

    // close files
    ofile.close();
    infile.close();
    
    cout << "\nCleaned trace";
    return completeTrace;
}


bool parseTraceFile()
{
    cout << "\nReading trace";
     
    TraceStep step;
    string traceCopy = cleanTraceFile() + "\n\n#end#";       // marker added to label end of string
    string line = "";
    bool endOfString = false;
           
    regex basicPattern ("\\s*(\\d+):\\s*proc\\s*(\\d+)\\s*\\((.+)\\:(\\d+)\\)\\s*(.+)\\:(\\d+)\\s*\\(state\\s*(\\d+)\\)\\s*\\[(.+)\\]");
    regex terminationPattern ("\\s*(\\d+):\\s*proc\\s*(\\d+)\\s*terminates");
    std::smatch capturedGroups;

    // iterate over entire trace and identify and store each step with all its info
    while(!endOfString) 
    {
        // get next line
        line = traceCopy.substr(0 , traceCopy.find('\n'));
        while (line == "")
        {
            traceCopy = traceCopy.substr(1); // skip line
            line = traceCopy.substr(0 , traceCopy.find('\n'));
        }
        
        if (line.find("#end#") != string::npos)
        {
            endOfString = true;
            break;
        }

        //checking

        // check for termination
        if(regex_search(line, capturedGroups, terminationPattern))
        {
//            cout << "\nTermination found\n";
            
            // store captured fields
            step.stepNumber = stoi(capturedGroups[1].str());
            step.processNumber = stoi(capturedGroups[2].str());
            step.instruction = "-end-";        
        }
        
        // check for normal step
        else if(regex_search(line, capturedGroups, basicPattern))
        {
//            cout << "\nNormal step found\n";

            // store captured fields
            step.stepNumber = stoi(capturedGroups[1].str());
            step.processNumber = stoi(capturedGroups[2].str());
            step.processName = capturedGroups[3].str();
            step.priority = stoi(capturedGroups[4].str());
            step.filename = capturedGroups[5].str();    
            step.lineNumber = stoi(capturedGroups[6].str());
            step.stateNumber = stoi(capturedGroups[7].str());
            step.instruction = capturedGroups[8].str();

            // extract variables
            traceCopy = traceCopy.substr(line.length()+1); // skip the whole match
            line = traceCopy.substr(0 , traceCopy.find('\n'));
            while (line != "")
            {
                step.globalVariables.insert(extractVariable(line));

                // get next line
                traceCopy = traceCopy.substr(line.length()+1); // skip the whole match
                line = traceCopy.substr(0 , traceCopy.find('\n'));
            }         
        }
 
        if(!step.isEmpty())
        {
            traceSteps.push_back(step);
        }
        
        step.clear();
        
        // advance trace
        traceCopy = traceCopy.substr(line.length()+1); // skip the whole match
    } 

    fprintVector(traceSteps , "tracesteps.txt");
    cout << "\nRead trace";
    return false;   
}

bool matchSteps(TraceStep step1 , TraceStep step2)
{
    cout << "\nMatching:\tinteractive (" << step1.lineNumber;
    cout << ") | trace(" << step2.lineNumber << ")\n";
    
    if((step1.instruction).find("-end-") != string::npos)   // is process termination step
    {
        cout << "\nMatched Termination:\tinteractive (" << step1.lineNumber;
        cout << ") | trace(" << step2.lineNumber << ")\n";    
        return step1.processNumber == step2.processNumber;
    }        
    
    else if(step1.processNumber == step2.processNumber && step1.filename == step2.filename && step1.lineNumber == step2.lineNumber)
    {
        return true;
    }
    
    else if((step1.instruction).find("ATOMIC") != string::npos)   // is process termination step
    {
        string all = step1.instruction;
        int possiblity = -1;
        all = all.substr(all.find(":")+1);
        all += ",";
        while(all.find(",") != string::npos)
        {
            possiblity = stoi(all.substr(0,all.find(",")));        
            if(step1.processNumber == step2.processNumber && step1.filename == step2.filename && possiblity == step2.lineNumber)
            {
                cout << "\nMatched Atomic:\tinteractive (" << possiblity;
                cout << ") | trace(" << step2.lineNumber << ")\n";    
                return true;
            }
            all = all.substr(all.find(",")+1);
        }
    }
    
    else
    {
        regex pattern ("goto\\s+(?!:)");
        std::smatch capturedGroups;
        if(regex_search(step1.instruction, capturedGroups, pattern))
        {
            /*
            //  if trace step is not goto, skip interactive step
            if(step2.instruction.find("goto") == string::npos)
                return false;
//            "goto L2:18,19->23->27,20->25->27"
            string inst = step1.instruction.substr(step1.instruction.find(":")+1);  // 18,19->23->27,20->25->27
            vector<string> targetChains = split(inst , ',');
            for (vector<string>::iterator c = targetChains.begin(); c != targetChains.end(); ++c)
            {                
                //  18,19->23->27,20->25->27
                string all = *c;
                //  18
                //  19->23->27
                //  20->25->27

                int possiblity = -1;
                all += "->";
                while(all.find("->") != string::npos)
                {
                    possiblity = stoi(all.substr(0 , all.find("->")));        
                    if(step1.processNumber == step2.processNumber && step1.filename == step2.filename && possiblity == step2.lineNumber)
                    {
                        cout << "\nMatched Goto:\tinteractive (" << possiblity;
                        cout << ") | trace(" << step2.lineNumber << ")\n";    
                        return true;
                    }
                    all = all.substr(all.find("->") + 1);
                }               
            }  */         
        }
    }
        
    return false;
}

bool matchDSteps(TraceStep step1 , TraceStep step2)
{
    cout << "\nMatching d_step:\tinteractive (" << step1.lineNumber;
    cout << ") | trace(" << step2.lineNumber << ")\n";

    if(step1.processNumber == step2.processNumber && step1.filename == step2.filename && step1.instruction == step2.instruction)
    {
        return true;
    }
    return false;
}

int findInCode(string str)
{
    // int found = false;
    size_t found;
    int count = 0;

    for (vector<string>::iterator i = inputCode.begin(); i != inputCode.end(); ++i)
    {
        found = (*i).find(str);
        if (found != string::npos)
        {
            // cout << "first 'needle' found at: " << int(found);
            return count;
        }
        count++;
    }
   
}

int determineScenario()
{
    string erroneousLine =  getCodeAtStepIndex(stepsWithChoices.back());  
    string erroneousInstruction =  traceSteps.at(stepsWithChoices.back()).instruction;  
    regex pattern;
    std::smatch captureGroups;
    bool isOne = true , isA = true;
    
    
    cout << "\nLine is " << erroneousLine;
    cout << "\nLine in trace is " << erroneousInstruction;
            
    //check if condition, with erroneousInstruction
    pattern = "^\\s*\\(.*\\)\\s*";
    if(regex_search(erroneousInstruction, captureGroups, pattern))
    {
        // check if condiiton or run() statemetnt, as trace places run() in parantheses as well
        pattern = "\\s*\\(run\\s*.*\\(.*\\).*\\)\\s*";
        if(!regex_search(erroneousInstruction, captureGroups, pattern))
        {
            isOne = false;
            cout << "\nCondition found";
        }
        else
        {
            cout << "\nRun command found";
        }
    }
    
    // check if on its own or as a choice (starts with "::") with erroneousLine
    pattern = "\\s*\\:\\:.*";
    if(regex_search(erroneousLine, captureGroups, pattern))
    {
        isA = false;
    }
    
    if(isOne && isA)
        return 1;
    if(isOne && !isA)
        return 2;
    if(!isOne && isA)
        return 3;
    if(!isOne && !isA)
        return 4;
}

void insertCheck()
{
    if(numberOfIterations > 0)
    {
        cout << "\noause here";
    }
    string erroneousLine =  getCodeAtStepIndex(stepsWithChoices.back()); 
    string synthesizedLine = "";
    regex pattern;
    std::smatch captureGroups;
    
    // determine scenario # of fix
    int scenario = determineScenario();
    cout << "\nScenario is " << scenario;
    
    // find out length of whitespace
        
//    // remove comment
//    if( string::npos != erroneousLine.find("/*"))
//    {
//        string beforeComment = erroneousLine.substr(0 , erroneousLine.find("/*"));
//        string afterComment = erroneousLine.substr(erroneousLine.find("*/")+2);
//        erroneousLine = beforeComment + afterComment;
//    }
    
    string varCheck = "!(";

    // get all global variables (Eta(i-1)) and add them to the guard
    for(auto elem : traceSteps.at(stepsWithChoices.back()-1).globalVariables)    // Eta of step before hybrid step
    {
       varCheck += elem.first + " == " + elem.second;
       varCheck += " && ";
    }
    varCheck = varCheck.substr(0 , varCheck.length()-4);    // remove trailing "&&"
    varCheck += ")";
    
    
    cout << "\nGuard is " << varCheck;
    
    switch(scenario)
    {
        case 1:     //  1a
            pattern = "(\\s*)(.*)";
            if(regex_search(erroneousLine, captureGroups, pattern))
            {
                synthesizedLine = captureGroups[1].str();   // leading whitespace        
                synthesizedLine += "atomic\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "{\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += " \t";                   // extra indentation inside atomic block
                // insert guard
                synthesizedLine += varCheck;                // Eta guard
                synthesizedLine += "\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += " \t";                   // extra indentation inside atomic block
                synthesizedLine += captureGroups[2].str();  // original code               
                synthesizedLine += "\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "}";
                
                setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);                 
            }
            break;
            
        case 2:     //  1b
            pattern = "(\\s*)\\:\\:(\\s*)(.*)";
            if(regex_search(erroneousLine, captureGroups, pattern))
            {
                synthesizedLine = captureGroups[1].str();   // leading whitespace
                synthesizedLine += "::";                
                synthesizedLine += captureGroups[2].str();  // whitespace after "::"
                synthesizedLine += "atomic\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "  ";                    // whitespace to account for "::" above
                synthesizedLine += captureGroups[2].str();  // whitespace after "::"
                synthesizedLine += "{\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "  ";                    // whitespace to account for "::" above
                synthesizedLine += captureGroups[2].str();  // whitespace after "::"
                synthesizedLine += "\t\t";                  // extra indentation inside atomic block
                // insert guard
                synthesizedLine += varCheck;                // Eta guard
                synthesizedLine += "\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "  ";                    // whitespace to account for "::" above
                synthesizedLine += captureGroups[2].str();  // whitespace after "::"
                synthesizedLine += "\t\t";                  // extra indentation inside atomic block
                synthesizedLine += captureGroups[3].str();  // original code               
                synthesizedLine += "\n";
                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "  ";                    // whitespace to account for "::" above
                synthesizedLine += captureGroups[2].str();  // whitespace after "::"
                synthesizedLine += "}";
                
                setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);             
            }
            break;
            
        case 3:     //  2a
            // TO DO : remove trailing whitespace after lines, which ends up printing before '&&'
//            pattern = "(\\s*)(.*\\))(.*)";
            pattern = "(\\s*)(.*)(\\s*)(;|->)";    // ends with ';' or '->'
            if(regex_search(erroneousLine, captureGroups, pattern))
            {               
                synthesizedLine += captureGroups[1].str();  // leading whitespace                          
                synthesizedLine += captureGroups[2].str();  // original check               
                
                // insert guard
                synthesizedLine += " &&\n";                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += varCheck;                // Eta guard
                synthesizedLine += "";
                
                synthesizedLine += captureGroups[3].str();  // whitespace after original condition
                synthesizedLine += captureGroups[4].str();  // succesor of check (e.g. : or -> or nothing)
                
                setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);                 
            }
            else    // ends without ; or ->
            {
                pattern = "(\\s*)(.*)";    // ends with ';' or '->'
                if(regex_search(erroneousLine, captureGroups, pattern))
                {               
                    synthesizedLine += captureGroups[1].str();  // leading whitespace                          
                    synthesizedLine += captureGroups[2].str();  // original check               

                    // insert guard
                    synthesizedLine += " &&\n";                
                    synthesizedLine += captureGroups[1].str();  // leading whitespace
                    synthesizedLine += varCheck;                // Eta guard
                    synthesizedLine += "";

                    setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);                 
                }
            }
            break;
            
        case 4:     //  2b
            pattern = "(\\s*)\\:\\:(\\s*)(.*\\))(.*)";
            // TO DO : remove trailing whitespace after lines, which ends up printing before '&&'
//            pattern = "(\\s*)(.*\\))(.*)";
            pattern = "(\\s*)\\:\\:(\\s*)(.*)(\\s*)(;|->)";    // ends with ';' or '->'
            if(regex_search(erroneousLine, captureGroups, pattern))
            {               
                synthesizedLine += captureGroups[1].str();  // leading whitespace                          
                synthesizedLine += "::";                    // "::"                        
                synthesizedLine += captureGroups[2].str();  // whitespace following "::"                        
                synthesizedLine += captureGroups[3].str();  // original check               
                
                // insert guard
                synthesizedLine += " &&\n";                
                synthesizedLine += captureGroups[1].str();  // leading whitespace
                synthesizedLine += "  ";                    // whitespace to account for "::" above                        
                synthesizedLine += captureGroups[2].str();  // whitespace following "::"                        
                synthesizedLine += varCheck;                // Eta guard
                synthesizedLine += "";
                
                synthesizedLine += captureGroups[4].str();  // whitespace after original condition
                synthesizedLine += captureGroups[5].str();  // succesor of check (e.g. : or -> or nothing)
                
                setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);                 
            }
            else    // ends without ; or ->
            {
                pattern = "(\\s*)(.*)";    // ends with ';' or '->'
                if(regex_search(erroneousLine, captureGroups, pattern))
                {               
                    synthesizedLine += captureGroups[1].str();  // leading whitespace                          
                    synthesizedLine += captureGroups[2].str();  // original check               

                    // insert guard
                    synthesizedLine += " &&\n";                
                    synthesizedLine += captureGroups[1].str();  // leading whitespace
                    synthesizedLine += varCheck;                // Eta guard
                    synthesizedLine += "";

                    setCodeAtStepIndex(stepsWithChoices.back() , synthesizedLine);                 
                }
            }
            break;            
    }
    
    cout << "\nSynthesized line:\n" << synthesizedLine;
}

string getCodeAtLine(int l)
{
    return inputCode[l-1];
}

void setCodeAtLine(int i , string s)
{
    correctredCode[i-1] = s;
}

string getCodeAtStepIndex(int l)
{
    return getCodeAtLine(traceSteps.at(l).lineNumber);
}

void setCodeAtStepIndex(int i , string s)
{
    setCodeAtLine(traceSteps.at(i).lineNumber , s);
}

void performCorection()
{
    cout << "\nPerforming correction";

    // print list of steps with nondetermistic choices, or quit if none exist
    if(stepsWithChoices.size() <= 0)
    {
        cout << "\n\nThe model is not correctable, as there it contains no nondeterminism that could be resolved";
        exit(0);

    }
    else if(stepsWithChoices.back() == 0)
    {
        cout << "\n\nThe model cannot be corrected automatically. The problem is in the very first step at line " << traceSteps.at(stepsWithChoices.back()).lineNumber << ". Recheck the initial variable evaluation.";
        exit(0);

    }
    cout << "\n\nThe following steps in the trace had nondeterministic choices:";
    printVector(stepsWithChoices);


    string erroneousLine = getCodeAtStepIndex(stepsWithChoices.back());  
    cout << "\nCode at line " << stepsWithChoices.back() << " is: .." << erroneousLine << "..\n";

    correctredCode = inputCode;

    insertCheck();

    string fname = infileName_original;
    fname = infileName_original.substr(0 , infileName_original.length()-4);
    fname += "_" + std::to_string(numberOfIterations) + ".pml";
    // cout << "\n\n fname is " << fname;
    fprintVector(correctredCode , fname);
    infileName = fname;
}

// consolidationg
void createFIFOs()
{
    // create named pipe for parent to child communication
    if(mkfifo(childInFifoName.c_str() , 0777) < 0)
    {
        // ignore if error is EEXIST
        if(errno != EEXIST)
        {
            cerr << "\nUnable to create stdin fifo. " << childInFifoName;
            cerr << strerror(errno);
            exit(errno);
        }
        else
        {
            cerr << "\nUsing existing stdin fifo. " << childInFifoName;
            cerr << strerror(errno);
        }
    }
    // create named pipe for child to parent communication
    if(mkfifo(childOutFifoName.c_str() , 0777) < 0)
    {
        // ignore if error is EEXIST
        if(errno != EEXIST)
        {
            cerr << "\nUnable to create stdout fifo. " << childOutFifoName;
            cerr << strerror(errno);
            exit(errno);
        }
        else
        {
            cerr << "\nUsing existing stdout fifo. " << childOutFifoName;
            cerr << strerror(errno);
        }
    }

}

void printRegexMatches(string text , regex pattern)
{
    std::smatch capturedGroups;
        if(regex_search(text, capturedGroups, pattern)) 
        {
            cout << "Match found\n";
            cout << "Number of captured groups = " << capturedGroups.size() << "\n";

            for (size_t i = 0; i < capturedGroups.size(); ++i) 
            {
                std::cout << i << ": '" << capturedGroups[i].str() << "'\n";
            }
    	} 
    	else 
    	{
            std::cout << "Match not found\n";
        }
}

void printTraceStep(TraceStep ts)
{
//      2:	proc  0 (:init::1) toy-maker.pml:8 (state 1)	[(run TM())]
//		Stage = 0
//		gvar = 0
//              
    cout << "\nTrace Step:\n";    
    cout << ts.stepNumber;
    cout << ":\tproc ";
    cout << ts.processNumber;
    cout << "\t(";
    cout << ts.processName;
    cout << ":";
    cout << ts.priority;
    cout << ")\t";
    cout << ts.filename;    
    cout << ":";
    cout << ts.lineNumber;
    cout << "\t(state ";
    cout << ts.stateNumber;
    cout << ")\t[";
    cout << ts.instruction;
    cout << "]";
    printVariableEvaluation(ts.globalVariables);
    cout << "\n";

}

void eprintTraceStep(TraceStep ts)
{              
    cerr << "\nTrace Step:\n";    
    cerr << ts.stepNumber;
    cerr << ":\tproc ";
    cerr << ts.processNumber;
    cerr << "\t(";
    cerr << ts.processName;
    cerr << ":";
    cerr << ts.priority;
    cerr << ")\t";
    cerr << ts.filename;    
    cerr << ":";
    cerr << ts.lineNumber;
    cerr << "\t(state ";
    cerr << ts.stateNumber;
    cerr << ")\t[";
    cerr << ts.instruction;
    cerr << "]";
    eprintVariableEvaluation(ts.globalVariables);
    cerr << "\n";
}

TraceStep getNextTraceStep(bool & hasEnded)
{
    traceStepCounter++;
    if((traceStepCounter) < traceSteps.size())
    {
        cout << "\nReturning trace step with line number " << traceSteps.at(traceStepCounter).lineNumber << "\n";
        return traceSteps.at(traceStepCounter);
    }
    else if((traceStepCounter) > traceSteps.size())
    {
        cerr << "\nERROR! Trace step counter has exceeded number of trace steps at " << traceStepCounter;
        cerr << "\nAborting.";
        exit(1);
    }
    hasEnded = true;
    cout << "\nEnd of trace. Returning empty trace step\n";
    return TraceStep();
}

std::pair<string , string> extractVariable(string line)
{
    int splt = line.find("=");
    return std::pair<string, string>(ltrim(line.substr(0,splt-1)), ltrim(line.substr(splt+2)));
}

// generate verifier: spin -a  toy-maker.pml
// compile verifier: gcc -DMEMLIM=1024 -O2 -DXUSAFE -DNOREDUCE -w -o pan pan.c
// run verifier: ./pan -m10000  -a
// run trace: spin -p -s -r -X -v -n123 -l -g -w -k toy-maker.pml.trail -u10000 toy-maker.pml > trace.txt

bool checkIfErrorTraceWritten()
{
    std::ifstream infile(panOutput.c_str());
    string target = "wrote ";
    target += infileName;
    target += ".trail";

    string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        
        if( std::string::npos != line.find(target) )
            return true;
    }

    return false;  
}

void clearGlobals()
{
        traceSteps.clear();
        traceIterator = 0;
        traceStepCounter = -1;
        inputCode.clear();
        correctredCode.clear();
        firstPromptCleaning = true;
        feedbackStep = false;
        stepsWithChoices.clear();
        inputCode.clear();
        correctredCode.clear();
        errorExists = true;  
}