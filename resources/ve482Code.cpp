#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

string* divideCmd(string cmd, int &cnt);
int processSimpleCmd(string cmd, int *preFd, int *postFd) ;
int processPipedCmd(string* cmd, int cmdCnt);
int getArgn(int & argn, string cmd);
int parseCmd(string* argu, string cmd);
int redirectCmd(string* argu, int argn, int &type, int &end);
int runCmd(string cmd, string* argu, int argn, int *preFd, int *postFd);

int main() {
	string cmdInput, *cmdSet;
	int cmdCnt;

	while (true) {
		cout << "ve482sh:" << get_current_dir_name() << "$ ";
		getline(cin, cmdInput);
		cmdSet = divideCmd(cmdInput, cmdCnt);
		
		if (cmdCnt == 1)
			processSimpleCmd(cmdInput, 0, 0);
		else
			processPipedCmd(cmdSet, cmdCnt);

		delete [] cmdSet;
	}
	return 0;
}

string* divideCmd(string cmd, int &cnt) {
	int len = cmd.size();
	cnt = 1;
	for (int i = 0; i < len; i++)
		if (cmd[i] == '|')
			cnt++;
	
	string* cmdSet = new string [cnt];
	for (int i = 0; i < cnt; i++) {
		int pos = cmd.find('|', 0);
		if (pos == -1) 
			cmdSet[i] = cmd;
		else {
			cmdSet[i] = cmd.substr(0, pos);
			cmd = cmd.substr(pos + 1, cmd.size() - pos + 1);
		}
	}

	return cmdSet;
}

int processPipedCmd(string *cmdSet, int cmdCnt) {
	int preFd[2], postFd[2];

	pipe(postFd);
	processSimpleCmd(cmdSet[0], 0, postFd);
	preFd[0] = postFd[0];
	preFd[1] = postFd[1];
	for (int i = 1; i < cmdCnt - 1; i++) {
		pipe(postFd);
		processSimpleCmd(cmdSet[i], preFd, postFd);
		preFd[0] = postFd[0];
		preFd[1] = postFd[1];
	}
	processSimpleCmd(cmdSet[cmdCnt - 1],preFd, 0);

	return 0;
}

int processSimpleCmd(string cmd, int *preFd, int *postFd) {
	if (cmd == "")
		return 0;

	string* argu;
	int argn;
	if (getArgn(argn, cmd))
		return 1;
	argu = new string [argn];
	parseCmd(argu, cmd);

	runCmd(cmd, argu, argn, preFd, postFd);
	return 0;
}

int getArgn(int &argn, string cmd) {
	int len = cmd.size();
	int inCnt = 0, outCnt = 0;
	string now = "";
	argn = 0;
	
	for (int i = 0; i < len; i++) {
		if (cmd[i] == ' ') {
			if (now != "") {
				argn++;
				now = "";
				inCnt = outCnt = 0;
			}
		}
		else if (cmd[i] == '<') {
			inCnt++;
			if ((inCnt > 1) || (outCnt > 0)) {
				cout << "Error: syntax error!" << endl;
				return 1;
			}		
			if (now != "")	
				argn++;
			now = "<";
		}
		else if (cmd[i] == '>') {
			outCnt++;
			if ((inCnt > 0) || (outCnt > 2)) {
				cout << "Error: syntax error!" << endl;
				return 1;
			}
			if (outCnt == 1) {				
				if (now != "")	
					argn++;
				now = ">";
			}
			else if (outCnt == 2) {
				now = ">>";
			}
		}
		else {
			if ((inCnt > 0) || (outCnt > 0)) {
				if (now != "")	
					argn++;
				now = "";
				inCnt = outCnt = 0;
			}	
			now += cmd[i];		
		}		
	}	

	if (now != "")
		argn++;
	
	return 0;
}

int parseCmd(string* argu, string cmd) {
	int len = cmd.size();
	int inCnt = 0, outCnt = 0;
	string now = "";
	int argn = 0;

	for (int i = 0; i < len; i++) {
		if (cmd[i] == ' ') {
			if (now != "") {
				argu[argn++] = now;
				now = "";
				inCnt = outCnt = 0;
			}
		}
		else if (cmd[i] == '<') {
			inCnt++;
			if ((inCnt > 1) || (outCnt > 0)) {
				cout << "Error: syntax error!" << endl;
				return 1;
			}	
			if (now != "")	
				argu[argn++] = now;
			now = "<";
		}
		else if (cmd[i] == '>') {
			outCnt++;
			if ((inCnt > 0) || (outCnt > 2)) {
				cout << "Error: syntax error!" << endl;
				return 1;
			}
			if (outCnt == 1) {				
				if (now != "")	
					argu[argn++] = now;
				now = ">";
			}
			else if (outCnt == 2) {
				now = ">>";
			}
		}
		else {
			if ((inCnt > 0) || (outCnt > 0)) {
				if (now != "")	
					argu[argn++] = now;
				now = "";
				inCnt = outCnt = 0;
			}	
			now += cmd[i];		
		}		
	}

	if (now != "")
		argu[argn++] = now;
	
	return 0;
}

int redirectCmd(string* argu, int argn, int &type, int &end) {
	type = 0;
	int posIn = 2048, posOut = 2048;
	for (int i = 0; i < argn; i++) {
		if (argu[i] == "<") {
			if ((type & 1) == 1) {
				cout << "Error: duplicated inputs!" << endl;
				return 1;
			}
			type |= 1;
			posIn = i;
		}
		else if (argu[i] == ">") {
			if ((type & 2) == 2) {
				cout << "Error: duplicated outputs!" << endl;
				return 1;
			}
			if ((type & 4) == 4) {
				cout << "Error: duplicated outputs!" << endl;
				return 1;
			}
			type |= 2;
			posOut = i;
		}
		else if (argu[i] == ">>") {
			if ((type & 2) == 2) {
				cout << "Error: duplicated outputs!" << endl;
				return 1;
			}
			if ((type & 4) == 4) {
				cout << "Error: duplicated outputs!" << endl;
				return 1;
			}
			type |= 4;
			posOut = i;
		}
	}

	if (type) {
		if (posIn + 1 == argn) {
			cout << "Error: more arguments required!" << endl;
			return 1;
		}

		if (posOut + 1 == argn) {
			cout << "Error: more arguments required!" << endl;
			return 1;
		}

		if (type & 1) {
			int fd = open(argu[posIn + 1].c_str(), O_RDONLY, S_IREAD|S_IWRITE);
			if (fd < 0) {
				cout << "Error: cannot open!" << endl;
				return 1;
			}
			dup2(fd, STDIN_FILENO);
		}
		
		if (type & 2) {
			int fd = open(argu[posOut + 1].c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
			if (fd < 0) {
				cout << "Error: cannot open!" << endl;
				return 1;
			}
			dup2(fd, STDOUT_FILENO);
		}

		if (type & 4) {
			int fd = open(argu[posOut + 1].c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
			if (fd < 0) {
				cout << "Error: cannot open!" << endl;
				return 1;
			}
			dup2(fd, STDOUT_FILENO);
		}
	}
	
	end = argn;
	end = (posIn < end) ? posIn : end;
	end = (posOut < end) ? posOut : end;

	return 0;
}


int runCmd(string cmd, string* argu, int argn, int *preFd, int *postFd) {
	if (argu[0] == "exit") {
		cout << "Bye~~~~~~~~" << endl; 
		delete [] argu;
		exit(0);
	} 
	else if (argu[0] == "cd") {
		if ((argn > 0) && (argu[1] == "~"))
			argu[1] = getenv("HOME");
		if ((argn > 1) && (chdir(argu[1].c_str()) < 0)) {
			cout << "Error: no such file or directory!" << endl;
			return 1;
		}
	}
	else {
		if (fork()) {
			int status;
			waitpid(-1, &status, 0);
		}
		else {	
			int type, end;
			if (redirectCmd(argu, argn, type, end))
				exit(1);
			
			if (!(type & 1) && preFd) {
				close(preFd[1]);
				if (preFd[0] != STDIN_FILENO) {
					dup2(preFd[0], STDIN_FILENO);
					close(preFd[0]);
				}
			}
		
			if (!(type & 2) && !(type & 4) && postFd) {
				close(postFd[0]);
				if (postFd[1] != STDOUT_FILENO) {
					dup2(postFd[1], STDOUT_FILENO);
					close(postFd[1]);
				}
			}			

			if (argu[0] == "echo") {
				if ((argu[1] != "<") && (argu[1] != ">") && (argu[1] != "<")) {
					cout << argu[1];
					for (int i = 2; i < argn; i++){	
						if ((argu[i] == "<") || (argu[i] == ">") || (argu[i] == ">>"))
							break;
						cout << ' ' << argu[i];
					}
				}
				cout << endl;
			}
			else {
				char** cArgu = new char* [end + 1];
				for (int i = 0; i < end; i++) {
					cArgu[i] = new char [argu[i].size()];	
					strcpy(cArgu[i], argu[i].c_str());
				}
				cArgu[end] = NULL;

				if (execvp(cArgu[0], cArgu) < 0) {
					dup2(0, STDERR_FILENO);
					cout << "Error: command \"" << argu[0] << "\" not found" << endl;
					exit(1);
				}
			}

			exit(0);
		}	
	}
	if (postFd)
		close(postFd[1]);
	return 0;
}





















