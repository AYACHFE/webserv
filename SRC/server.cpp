#include "../INC/server.hpp"

std::map<int, httpRequest> fdMapRead;
std::map<int, httpResponse> fdMapWrite;
std::map<int, Server> servers_sockets;
fd_set theFdSetRead[NBOFCLIENTS];
fd_set theFdSetWrite[NBOFCLIENTS];

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
		close(commSocket);
		fdMapWrite.erase(commSocket);
		return 0;
	}
	else if(returnNumber == 2)
	{
		fdMapRead.insert(std::make_pair(commSocket, httpRequest(commSocket)));
		fdMapWrite.erase(commSocket);
		return 0;
	}
	else 
		return 0;
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
		select(getMaxFd()+1, theFdSetRead, theFdSetWrite, NULL, &timout);
		
		for(std::map<int, Server>::iterator it = servers_sockets.begin(); it != servers_sockets.end(); it++)
		{
			if(FD_ISSET(it->first, theFdSetRead))
			{
				acceptNewConnections(it->first);
				goto debute;
			}
		}
		if(1)
		{
			for (std::map<int, httpRequest>::iterator it = fdMapRead.begin(); it != fdMapRead.end(); it++)
			{
				if(FD_ISSET(it->first, theFdSetRead))
				{
					if(readTheRequest(it) == 0)
						goto debute;
				}
			}
			for (std::map<int, httpResponse>::iterator it = fdMapWrite.begin(); it != fdMapWrite.end(); it++)
			{
				if(FD_ISSET(it->first, theFdSetWrite))
				{
					if(writeOnSocket(it) == 0)
						goto debute;
				}
			}
		}
	}

	return 0;
}