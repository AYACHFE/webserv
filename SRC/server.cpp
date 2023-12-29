/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eboulhou <eboulhou@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/29 09:45:04 by eboulhou          #+#    #+#             */
/*   Updated: 2023/12/29 20:29:18 by eboulhou         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../INC/server.hpp"

std::map<int, httpRequest> fdMapRead;
std::map<int, httpResponse> fdMapWrite;
std::map<int, Server> servers_sockets;
std::vector<int> deleteReadFd;
std::vector<int> deleteWriteFd;
fd_set theFdSetRead[NBOFCLIENTS];
fd_set theFdSetWrite[NBOFCLIENTS];


int  readTheRequest(std::map<int, httpRequest>::iterator& it)
{
	int commSocket;
	char buffer[150];
	int size_readed;
	string request;
	static int content_length = -1;



	commSocket = it->first;
	bzero(buffer, 150);
	size_readed = recv(commSocket, buffer, 150, 0);
	if(size_readed == -1)
	{
		cerr << "error at reading from socket"<< endl;
		exit(1);
	}
	else if(size_readed == 0)
	{
		cout << "connection ended"<< endl;
		close(commSocket);
		// fdMapRead.erase(commSocket);
		deleteReadFd.push_back(commSocket);
		return 0;
	}
	else 
	{
		it->second.request = it->second.request + string(buffer);
			request = it->second.request;
		// cout << "("<< it->second.method <<")"<< endl;
		if(it->second.method.empty())
		{
			size_t pos = request.find(" ");
			if(pos != string::npos)
				it->second.method = request.substr(0, pos);
		}
		size_t posofend;
		if(it->second.method == "POST" && (posofend = request.find("\r\n\r\n")) != string::npos)
		{
			if(content_length == -1)
			{
				size_t pos = request.find("Content-Length: ");
				if(pos != string::npos)
				{
					pos += 16;
					string st = request.substr(pos, request.find("\n", pos) - pos);
					content_length = std::atoi(st.c_str());
				}
			}
			if(content_length != -1 &&  (size_t)content_length ==  request.length() - (posofend + 4))
			{
				// cout << "full request received POST method"<< endl;
				it->second.generate_response();
				fdMapWrite.insert(std::make_pair(commSocket, httpResponse(it->second)));
				fdMapWrite[commSocket].setData();
				// fdMapRead.erase(commSocket);
				deleteReadFd.push_back(commSocket);
			}
				return 0;
		}
		if(request.size() > 4  && request.substr(request.size() - 4) == "\r\n\r\n")
		{
			// cout << "full request received with GET method"<<endl;
			it->second.generate_response();
			fdMapWrite.insert(std::make_pair(commSocket, httpResponse(it->second)));
			fdMapWrite[commSocket].setData();
			// fdMapRead.erase(commSocket);
			deleteReadFd.push_back(commSocket);
			return 0;
		}
		
	// cout << "("<< request << ")"<< endl;
	
	}
	// cout << "impossible to reach : "<< content_length << " " << size_readed << endl;
	return 0;
}

int  readTheRequest(std::map<int, httpRequest>::iterator& it);
int getMaxFd()
{
	int tmp = -1;
    if(fdMapRead.size() >= 1)
	{
		//PARSE: TRY TO USE RBEGIN() INSTEAD OF END
		std::map<int, httpRequest>::iterator it = fdMapRead.end();
		it--;
		if( it->first > tmp)
			tmp = it->first;
	}
	if(fdMapWrite.size() >= 1)
	{
		std::map<int, httpResponse>::iterator it = fdMapWrite.end();
		it--;
		if( it->first > tmp)
			tmp = it->first;
	}

    return tmp;
}

void refresh_fd_set(fd_set *fdRead, fd_set *fdWrite)
{
    // FD_ZERO(fdRead);
	bzero(fdRead, sizeof(fdRead) * NBOFCLIENTS);
	bzero(fdWrite, sizeof(fdWrite) * NBOFCLIENTS);

	for (std::vector<int>::iterator it = deleteReadFd.begin(); it != deleteReadFd.end(); it++)
		if(fdMapRead.find(*it) != fdMapRead.end())
			fdMapRead.erase(*it);
	for (std::vector<int>::iterator it = deleteWriteFd.begin(); it != deleteWriteFd.end(); it++)
		if(fdMapWrite.find(*it) != fdMapWrite.end())
			fdMapWrite.erase(*it);
	
	deleteReadFd.clear();
	deleteWriteFd.clear();
    for (std::map<int, httpRequest>::iterator it = fdMapRead.begin(); it != fdMapRead.end(); it++)
    {
        FD_SET(it->first, fdRead);
    }
    for (std::map<int, httpResponse>::iterator it = fdMapWrite.begin(); it != fdMapWrite.end(); it++)
    {
        FD_SET(it->first, fdWrite);
    }
}


int connectSockets(parceConfFile cf)
{
	int port;
	for (int i = 0; i < cf.server_nb ; i ++)
	{
		port = std::atoi(cf.server[i].listen[0].c_str());
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd == -1)
		{
			cerr << "Failed to Create Socket"<< endl;
			return 1;
		}
		struct sockaddr_in address;
		// socklen_t socket_lenght  = sizeof(address);
		bzero(&address, sizeof(struct sockaddr_in));
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		address.sin_addr.s_addr = INADDR_ANY;
		int optval = 1;
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
			cerr << "Error setting socket options" << endl;
			exit(1);
		}

		// Set the socket to non-blocking mode
		int flags = fcntl(sockfd, F_GETFL, 0);
		if (flags == -1) {
			cerr << "Can't get flags for socket" << endl;
			exit(1);
		}

		flags |= O_NONBLOCK;
		if (fcntl(sockfd, F_SETFL, flags) == -1) {
			cerr << "Can't set socket to non-blocking mode" << endl;
			exit(1);
		}

		if(bind(sockfd, (struct sockaddr*)&address, sizeof(address)) == -1)
		{
			cerr << "Failed to Bind"<< endl;
			exit(1);
		}

		if(listen(sockfd, 10) == -1)
		{
			cerr << "Failed to listen"<< endl;
			exit(1);
		}

		FD_ZERO(theFdSetRead);
		servers_sockets[sockfd] = cf.server[i];

		// cout <<"test this shit : " <<  servers_sockets[sockfd]->error_pages[0]<< endl;
		// cout <<"test this shit : " << servers_sockets[sockfd]->location[0].path << endl;

		fdMapRead.insert(std::make_pair(sockfd, httpRequest(-1, -1)));
	}
	return 1;
}

void acceptNewConnections(int sockfd)
{
	int datasocket = accept(sockfd, NULL, NULL);
		if(datasocket == -1)
		{
			cerr << "accept error" << endl;
			exit(1);
		}
		int optval =1 ;
		if(setsockopt(datasocket, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)) == -1)
		{
			cerr << "set Socket Options error"<<endl;
			exit(1);
		}
		int flags = fcntl(datasocket, F_GETFL, 0);
		if (flags == -1) {
			cerr << "Can't get flags for socket" << endl;
			exit(1);
		}
		flags |= O_NONBLOCK;
		if (fcntl(datasocket, F_SETFL, flags) == -1) {
			cerr << "Can't set socket to non-blocking mode" << endl;
			exit(1);
		}
		fdMapRead.insert(std::make_pair(datasocket, httpRequest(datasocket, sockfd)));

		
}


int writeOnSocket(std::map<int, httpResponse>::iterator& it)
{
	int commSocket, returnNumber;

	commSocket = it->first;			
	returnNumber = it->second.sendChunk();
	if (returnNumber == 1)
	{
		cout <<"erase 1"<<endl;
		close(commSocket);
		deleteWriteFd.push_back(commSocket);
		return 0;
	}
	else if(returnNumber == 2)
	{
		fdMapRead.insert(std::make_pair(commSocket, httpRequest(commSocket, it->second.server_socket)));
		cout << "erase 2"<< endl;
		deleteWriteFd.push_back(commSocket);
		return 0;
	}
	else
	{
		return 1;
	}
}

int main()
{
	parceConfFile cf;
	parce_conf_file(cf);

	struct timeval timout;
	timout.tv_sec = 5;
	timout.tv_usec = 0;
	connectSockets(cf);

	while (1)
	{
		debute:
		refresh_fd_set(theFdSetRead, theFdSetWrite);
		int ret = select(getMaxFd()+1, theFdSetRead, theFdSetWrite, NULL, &timout);
		cout << "=============ret : " << ret << endl;
		if(ret == -1)
		{
			cout << "select failed!!"<< endl;
			return 1;
		}
		// else if(ret == 0)
		// {
		// 	cout << "timeout for all the "<< endl;
		// 	for (std::map<int, httpRequest>::iterator it = fdMapRead.begin(); it != fdMapRead.end(); it++)
		// 	{
		// 		close(it->first);
		// 		deleteReadFd.push_back(it->first);
		// 	}
		// 	for (std::map<int, httpResponse>::iterator it = fdMapWrite.begin(); it != fdMapWrite.end(); it++)
		// 	{
		// 		close(it->first);
		// 		deleteWriteFd.push_back(it->first);
		// 	}
		// 	goto debute;
		// }//TODO: SHOULD NOT ERASE THE SERVER SOCKETS OF CONNECTIONS
		
		for(std::map<int, Server>::iterator it = servers_sockets.begin(); it != servers_sockets.end(); it++)
		{
			if(FD_ISSET(it->first, theFdSetRead))
			{
				cout << "connect" << endl;
				acceptNewConnections(it->first);
				goto debute;
			}
		}
		for (std::map<int, httpRequest>::iterator it = fdMapRead.begin(); it != fdMapRead.end(); it++)
		{
			if(FD_ISSET(it->first, theFdSetRead))
			{
				cout << "read "<< it->first << endl;
				readTheRequest(it);

			}
		}
		
		for (std::map<int, httpResponse>::iterator it = fdMapWrite.begin(); it != fdMapWrite.end(); it++)
		{
			if(FD_ISSET(it->first, theFdSetWrite))
			{
				cout << "write " << it->first << endl;
				writeOnSocket(it);

			}
		}
	}

	return 0;
}