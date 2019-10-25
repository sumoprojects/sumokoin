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
#include <sstream>

using namespace std;  

const std::string flagmsg = "(Type --help for list of options / press ENTER for none)";

  void show_menu()
  {     
    cout << endl << endl <<
    "-----------------------------------------------------" << std::endl <<
    "       SUMOKOIN BLOCKCHAIN UTILITIES LAUNCHER        " << std::endl <<
    "-----------------------------------------------------" << std::endl 
     << "1. Import blockchain from file " << endl
     << "2. Export blockchain to file " << endl
     << "3. Print blockchain statistics " << endl
     << "4. Print blockchain usage " << endl
     << "5. Check blockchain ancestry " << endl
     << "6. Check blockchain depth " << endl
     << "7. Mark spent outputs " << endl
     << "8. Prune blockchain " << endl
     << "9. Prune blockchain known spent data " << endl
     << "Press Ctrl^C at anytime to EXIT " << endl;
  }
  //---------------------------------------------------------------
  int import_chain()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-import.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-import " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int export_chain()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-export.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-export " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int stats()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-stats.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-stats " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int usage()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-usage.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-usage " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int ancestry()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-ancestry.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-ancenstry " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int depth()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-depth.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-depth " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int mark()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-mark-spent-outputs.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-mark-spent-outputs " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int prune()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-prune.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-prune " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int prune_spent()
  {
#ifdef _WIN32
   cout << "Input flags: " << endl << flagmsg << endl;
   char flagsw[250];
   cin.ignore();
   std::cin.getline(flagsw, 250);
   std::ostringstream commandw;
   commandw << "sumo-blockchain-prune-known-spent-data.exe " << flagsw;
   const std::string commandfw = commandw.str();
   const char *resultw = commandfw.c_str();
   return std::system(resultw);
#endif

   cout << "Input flags: " << endl << flagmsg << endl;
   char flags[250];
   cin.ignore();
   std::cin.getline(flags, 250);
   std::ostringstream command;
   command << "./sumo-blockchain-prune-known-spent-data " << flags;
   const std::string commandf = command.str();
   const char *result= commandf.c_str();
   return std::system(result);
  }
  //---------------------------------------------------------------
  int main()
  {    
   int choice;
    do
    {
     show_menu(); 
     cin>>choice;
     switch(choice)
      {
       case 1: import_chain();
       break;
       case 2: export_chain();
       break;
       case 3: stats();
       break;
       case 4: usage();
       break;
       case 5: ancestry();
       break;
       case 6: depth();
       break;
       case 7: mark();
       break;
       case 8: prune();
       break;
       case 9: prune_spent();
       break;
      }
    }
    while(choice > 0);
    return 0;
  }
