/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parceConfFile.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eboulhou <eboulhou@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/15 11:27:53 by aachfenn          #+#    #+#             */
/*   Updated: 2023/12/27 15:38:20 by eboulhou         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

using std::cout;
using std::endl;
using std::string;
using std::vector;

class Location {

	public:
	string path;
	string default_file;
	vector<string> methods;
	string cgi_bin;
	string cgi_extension;
		Location() {}
		~Location() {}
};

class Server {
	public:
		vector<string> listen;
		string server_name;
		vector<string> error_pages;
		double client_body_size;
		string root;
		vector<Location> location;
		int location_nb;
		
		Server();
		~Server();
		//copy construcot 
		Server(const Server& obj);
		//copy assignment operator
		Server& operator=(const Server& obj);
		
};

class parceConfFile
{
		vector<string> data;
		vector<string>::iterator it_data;
	public:
		int server_nb;
		vector<Server> server;
		
		parceConfFile();
		~parceConfFile();
		void read_conf_file();
		void fill_data();
		void my_location(Server &serv);
		void print_data();
		string my_trim(string str);
		void check_ifdata_isnot_empty();
		void check_ifdata_is_valid();

};

void parce_conf_file(parceConfFile &parce);