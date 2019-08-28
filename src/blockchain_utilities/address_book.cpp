// Copyright (c) 2017-2019, Sumokoin Project										
// Copyright 2017 by Ali Mirjamali used code structure from
// https://github.com/alimirjamali/EDUCATIONAL-CPP-Phonebook/blob/master/phonebook.cpp          		

//  This program is free software: you can redistribute it and/or modify	
//  it under the terms of the GNU General Public License as published by	
//  the Free Software Foundation, either version 3 of the License, or		
//  (at your option) any later version.						
										
//  This program is distributed in the hope that it will be useful,		
//  but WITHOUT ANY WARRANTY; without even the implied warranty of		
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		
//  GNU General Public License for more details.				

#include <iostream>
#include <string>
#include <fstream>

using namespace std;

const char* addressbook_filename = "address_book.txt";

enum groups {Exchanges, Friends, Eshops, VIP, Others};

struct address_info
{
	
	string address_number;
	string address_name;
	groups address_group;
	
	address_info* next_element=NULL;
};

address_info* addressbook=NULL;

unsigned int N_entries = 0;

address_info* look_up[26] = {NULL};

void upcase (string&);			// This is the function prototype which would convert an string to uppercase
address_info* create_entry();		// This is the function prototype which gets the input from user and stores in into a new address object.
void address_insert(address_info*);	// This is the function prototype which inserts the new address object into the addressbook
// The below function is used for both point C & D of project. It will retrun NULL (zero) if it does not find a matching address
address_info* contact_find(string);	// This is the function prototype to search and find a contact object based on name
void address_show(address_info);	// This is the function prototype to show the information of an address object
void address_find_show_delete();	// This is the function prototype to find and delete addresses (point C of project)
void address_find_show();		// This is the function prototype to find and show a name in the addressbook (plus some Error handling)
void addressbook_save();			// This is the function prototype to store data into file (point E & F of project)
void addressbook_load();			// This is the function prototype to load the data file (optional, was not asked in the project)

int main()
{
	
	addressbook_load();

	char userchoice;
	do
	{
		cout << endl;
		cout << "a. Create a new entry." << endl;
		cout << "b. Delete an entry." << endl;
		cout << "c. Find an entry and show its information. " << endl;
		cout << "d. Save the whole addressbook in a file. " << endl;
		cout << "e. Exit. " << endl;
		cout << endl << "Please enter your choice: ";
		cin >> userchoice;
		switch (userchoice)
		{

		case 'a': case 'A':
			address_insert(create_entry());
			break;

		case 'b': case 'B':
			address_find_show_delete();
			break;

		case 'c': case 'C':
			address_find_show();
			break;

		case 'd': case 'D': case 'e': case 'E':
			addressbook_save();
			break;
		// Otherwise we have an invalid choice and should show an error message
		default:
			cout << endl << "!!! Invalid choice !!!" << endl;
		}

	} while ((userchoice != 'e') && (userchoice != 'E'));
	cout << "Exiting..." << endl;
	return 0;
}

	void upcase(string &str)
{
	for (unsigned int i=0; i<str.length(); i++) str[i]=toupper(str[i]);
}

	address_info* create_entry()
{
	
	address_info* new_entry = new address_info;
	cout << "Please enter identifying name: ";
	cin.ignore();	
	getline (cin,new_entry->address_name);
	upcase(new_entry->address_name);
	cout << "Please enter Sumokoin address: ";
	getline (cin,new_entry->address_number);
	
	cout << "Please enter group" << endl;
	cout << "1, x or X = Exchanges" << endl;
	cout << "2, f or F = Friends" << endl;
	cout << "3, e or E = Eshops" << endl;
	cout << "4, v or V = VIP" << endl;
	cout << "or enter any other character for (Others) group" << endl;
	cout << "Your choice: ";
	char groupchoice;
	cin >> groupchoice;
	switch (groupchoice)
	{
	case '1': case 'x': case 'X':
		new_entry->address_group=Exchanges;
		break;
	case '2': case 'f': case 'F':
		new_entry->address_group=Friends;
		break;
	case '3': case 'e': case 'E':
		new_entry->address_group=Eshops;
		break;
	case '4': case 'v': case 'V':
		new_entry->address_group=VIP;
		break;
	default:
		new_entry->address_group=Others;
	}
	new_entry->next_element=NULL;
	cout << "New address successfully created." << endl;
	return new_entry;
}

void address_insert(address_info* newaddress)
{
	
	if (N_entries==0 || addressbook->address_name.compare(newaddress->address_name)>0)
	{
		newaddress->next_element=addressbook;
		addressbook=newaddress;
	}
	else
	{
		address_info* previous=addressbook;
		address_info* next=addressbook->next_element;
		while(next!=NULL)
		{
			if (newaddress->address_name.compare(next->address_name)<0) break;
			previous=next;
			next=next->next_element;
		}
		previous->next_element=newaddress;
		newaddress->next_element=next;		
	}	
	N_entries++;
}

address_info* address_find(string Name)
{
	address_info* address = addressbook;
	while (address != NULL)
	{
		if (Name.compare(address->address_name)==0)
			return address;
		else
			address=address->next_element;
	}
	return NULL;
}


void address_show(address_info* address)
{
	cout << "Address Name: " << address->address_name << endl;
	cout << "Sumokoin address: " << address->address_number << endl;
	cout << "Address Group: ";
	switch (address->address_group)
	{
		case Exchanges: cout << "EXCHANGES" << endl; break;
		case Friends: cout << "FRIENDS" << endl; break;
		case Eshops: cout << "E-SHOPS" << endl; break;
		case VIP: cout << "VIP" << endl; break;
		default: cout << "OTHERS" << endl;
	}
}

void address_find_show_delete()
{
	cout << "Please enter address name to be deleted: ";
	string Name;
	cin.ignore();
	getline (cin,Name);
	upcase(Name);
	cout << endl;
	address_info* address = address_find(Name);
	if(address != NULL)
	{
		cout << "The following address is going to be deleted:" << endl;
		address_show(address);
		cout << "Enter Y/y to confirm delete:";
		char confirm;
		cin >> confirm;
		if (confirm == 'Y' || confirm == 'y')
		{
			if (address==addressbook)
			{
				addressbook=addressbook->next_element;
			}
			else
			{
				address_info* previous=addressbook;
				while(previous->next_element != address) previous=previous->next_element;
				previous->next_element=address->next_element;				
			}
			
			delete address;
			N_entries--;
			cout << "Address successfully deleted" << endl;
		}
		else cout << "Did not delete address." << endl;
	}
	else cout << "Did not found " << Name << " in addressbook!!!" << endl;
}


void address_find_show()
{
	cout << "Please enter address name to be searched for: ";
	string Name;
	cin.ignore();
	getline (cin,Name);
	cout << endl;
	upcase(Name);
	address_info* address = address_find(Name);
	if(address != NULL)
		address_show(address);
	else
		cout << "Did not found " << Name << " in addressbook!!!" << endl;
}

void addressbook_save()
{
	ofstream addressbook_file;
	
	addressbook_file.open(addressbook_filename);
	cout << "Writing " << N_entries << " records to " << addressbook_filename << " ..." << endl;
	
	addressbook_file << N_entries << endl;
	address_info* current_item=addressbook;
	while (current_item != NULL)
	{
		
		addressbook_file << "********************" << endl;
		
		addressbook_file << current_item->address_name << endl;
		addressbook_file << current_item->address_number << endl;
		switch (current_item->address_group)
		{
			case Exchanges: addressbook_file << "EXCHANGES" << endl; break;
			case Friends: addressbook_file << "FRIENDS" << endl; break;
			case Eshops: addressbook_file << "E-SHOPS" << endl; break;
			case VIP: addressbook_file << "VIP" << endl; break;
			default: addressbook_file << "OTHERS" << endl;
		}
		current_item = current_item->next_element;
	}
	addressbook_file.close();
}

void addressbook_load()
{
	ifstream addressbook_file;
	addressbook_file.open(addressbook_filename);
	if(!addressbook_file.is_open())
	{
		cout << "Addressbook file could not be openned !!!" << endl;
	} else
	{
		addressbook_file >> N_entries;
		address_info** previous=&addressbook;
		string text;
		// Skiping a carriage return here (because of previous reading of Integer number)
		getline (addressbook_file,text);
		cout << "Reading " << N_entries << " of address records..." << endl;
		for (unsigned int i=0;i<N_entries;i++)
		{
			address_info* new_entry = new address_info;
			
			getline (addressbook_file,text);
			getline (addressbook_file,new_entry->address_name);
			getline (addressbook_file,new_entry->address_number);
			getline (addressbook_file,text);
			if      (text.compare("EXCHANGES") == 0)  new_entry->address_group=Exchanges;
			else if (text.compare("FRIENDS") == 0)	new_entry->address_group=Friends;
			else if (text.compare("E-SHOPS")==0)	new_entry->address_group=Eshops;
			else if (text.compare("VIP") == 0)	new_entry->address_group=VIP;
			else new_entry->address_group=Others;
			new_entry->next_element=NULL;
			*previous=new_entry;
			previous=&new_entry->next_element;
		}
		addressbook_file.close();
	}
}
