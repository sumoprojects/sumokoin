// Copyright (c) 2014-2019, Sumo Projects
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace std;     

void showMenu()
{     
    cout << endl << endl <<
    "\033[1;36m -----------------------------------------------------  \033[0m" << std::endl <<
    "\033[1;36m        SUMOKOIN BLOCKCHAIN UTILITIES LAUNCHER          \033[0m" << std::endl <<
    "\033[1;36m -----------------------------------------------------  \033[0m" << std::endl 
     << "\033[1;32m 1. Import blockchain from file \033[0m" << endl
     << "\033[1;32m 2. Export blockchain to file \033[0m" << endl
     << "\033[1;32m 3. Print blockchain statistics \033[0m" << endl
     << "\033[1;32m 4. Print blockchain usage \033[0m" << endl
     << "\033[1;32m 5. Check blockchain ancestry \033[0m" << endl
     << "\033[1;32m 6. Check blockchain depth \033[0m" << endl
     << "\033[1;32m 7. Mark spent outputs \033[0m" << endl
     << "\033[1;32m 8. Prune blockchain \033[0m" << endl
     << "\033[1;32m 9. Prune blockchain known spent data \033[0m" << endl
     << "\033[1;33m Press Ctrl^C at anytime to EXIT \033[0m" << endl;
}

int Import()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-import.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-import " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Export()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-export.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-export " + flags;
const char *result = command.c_str();
return std::system(result);
}

int Stats()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-stats.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-stats " + flags;
const char *result = command.c_str();
return std::system(result);
}

int Usage()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-usage.exe " + flagsw;
 const char *resultw= commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-usage " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Ancestry()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;;
 cin >> flags;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-ancestry.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-ancestry " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Depth()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-depth.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-depth " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Mark()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-mark-spent-outputs.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-mark-spent-outputs " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Prune()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-prune.exe " + flagsw;
 const char *resultw = commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-prune " + flags;
const char *result= command.c_str();
return std::system(result);
}

int Prunespent()
{
std::string flags;
std::string flagsw;
std::string command;
std::string commandw;

#ifdef _WIN32
 cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
 cin >> flagsw;
 std::getline(std::cin, flagsw);
 commandw = "sumo-blockchain-prune-known-spent-data.exe " + flagsw;
 const char *resultw= commandw.c_str();
 return std::system(resultw);
#endif

cout << "\033[1;33mInput flags: \033[0m" << endl << "(Type:'flags' plus your option (e.g flags --block-start=10 --block-stop=100) / 'none' plus ENTER for none / 'flags --help' for list of options)" << endl;
cin >> flags;
std::getline(std::cin, flags);
command = "./sumo-blockchain-prune-known-spent-data " + flags;
const char *result= command.c_str();
return std::system(result);
}



int main()
{
	int choice;
	do
	{
		showMenu(); 
		cin>>choice;

		switch(choice)
		{
			case 1:
				Import();
			break;

                        case 2:
				Export();
			break;

                        case 3:
				Stats();
			break;

			case 4:
				Usage();
			break;

                        case 5:
                                Ancestry();
                        break;

                        case 6:
                                Depth();
                        break;

                        case 7:
                                Mark();
                        break;

                        case 8:
                                Prune();
                        break;

                        case 9:
                                Prunespent();
                        break;

		}
	}
while(choice > 0);
	return 0;
}
