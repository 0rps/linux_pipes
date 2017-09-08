#include <iostream>
#include <string>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>

#include <list>

struct Command {
    std::string name;
    std::string param;
};

void exec_first_command(Command cmd);
void exec_next_command(std::list<Command> &_list, int ifd);
void exec_last_command(Command cmd, int ifd);



void exec_command(Command *cmd) {
    if (cmd->param.size() == 0) {
        execlp(cmd->name.c_str(), cmd->name.c_str(), NULL);
    } else {
        execlp(cmd->name.c_str(), cmd->name.c_str(), cmd->param.c_str(), NULL);
    }
}

void exec_first_command(std::list<Command> &_list) {
    if (_list.size() == 1) {
        exec_last_command(_list.front(), -1);
    }

    Command cmd = _list.front();
    _list.pop_front();

    int pfd[2];
    pipe(pfd);

    if (fork()) {
        close(pfd[1]);
        exec_next_command(_list, pfd[0]);
    } else {
        close(STDOUT_FILENO);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        close(pfd[0]);

        exec_command(&cmd);
    }
}


void exec_next_command(std::list<Command> &_list, int ifd) {

    Command cmd = _list.front();
    _list.pop_front();

    if (_list.size() == 0) {
        exec_last_command(cmd, ifd);
        return;
    }

    int pfd[2];
    pipe(pfd);

    if (fork()) {
        close(ifd);
        close(pfd[1]);

        exec_next_command(_list, pfd[0]);
    } else {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        dup2(ifd, STDIN_FILENO);
        dup2(pfd[1], STDOUT_FILENO );
        close(ifd);
        close(pfd[0]);
        close(pfd[1]);

        exec_command(&cmd);
    }

}

void exec_last_command(Command cmd, int ifd) {

    int filefd = open("/home/box/result.out", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (ifd != -1) {
        close(STDIN_FILENO);
        dup2(ifd, STDIN_FILENO );
        close(ifd);
    }

    close(STDOUT_FILENO);
    dup2(filefd, STDOUT_FILENO);
    close(filefd);

    exec_command(&cmd);
}

std::list<Command> parseString(std::string &cmdString) {
    cmdString.push_back(' ');

    std::list<Command> cmdList;
    Command curCmd;
    std::string curStr;
    for (int i = 0; i < cmdString.size(); i++) {
        char curChar = cmdString.c_str()[i];
        if (curChar == ' ') {
            if (curStr.size() > 0) {
                if (curCmd.name.size() > 0) {
                    curCmd.param = curStr;
                } else {
                    curCmd.name = curStr;
                }
                curStr.clear();
            }
        } else if (curChar == '|') {
                cmdList.push_back(curCmd);
                curCmd.name.clear();
                curCmd.param.clear();
                curStr.clear();

        } else {
            curStr.push_back(curChar);
        }
    }

    if (curCmd.name.size() > 0) {
        cmdList.push_back(curCmd);
    }

    return cmdList;
}

int main(int argc, char** argv)
{
    std::string cmdString;
    std::getline(std::cin, cmdString);

    std::list<Command> list = parseString(cmdString);

    if (list.size() > 0) {
        exec_first_command(list);
    }

    return 0;
}

