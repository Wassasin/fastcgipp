//! \file transceiver.cpp Defines member functions for Fastcgipp::Transceiver
/***************************************************************************
* Copyright (C) 2007 Eddie                                                 *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/


#include <fastcgi++/transceiver.hpp>

int Fastcgipp::Transceiver::transmit()
{
	while(1)
	{{
		Buffer::SendBlock sendBlock(buffer.requestRead());
		if(sendBlock.size)
		{
			ssize_t sent = write(sendBlock.fd, sendBlock.data, sendBlock.size);
			if(sent<0)
			{
				if(errno==EPIPE)
				{
					epoll_ctl(epollFd, EPOLL_CTL_DEL, sendBlock.fd, NULL);
					sent=sendBlock.size;
				}
				else if(errno!=EAGAIN) throw Exceptions::FastcgiException("Error writing to file descriptor");
			}

			buffer.freeRead(sent);
			if(sent!=sendBlock.size)
				break;
		}
		else
			break;
	}}

	return buffer.empty();
}

void Fastcgipp::Transceiver::Buffer::secureWrite(size_t size, Protocol::FullId id, bool kill)
{
	writeIt->end+=size;
	if(minBlockSize>(writeIt->data.get()+Chunk::size-writeIt->end) && ++writeIt==chunks.end())
	{
		chunks.push_back(Chunk());
		--writeIt;
	}
	frames.push(Frame(size, kill, id));
}

bool Fastcgipp::Transceiver::handler()
{
	using namespace std;
	using namespace Protocol;

	bool transmitEmpty=transmit();

	epoll_event event;
	int retVal=epoll_wait(epollFd, &event, 1, 0);
	if(retVal==0)
	{
		if(transmitEmpty) return true;
		else return false;
	}
	if(retVal<0) throw Exceptions::FastcgiException("Epoll Error");

	if(event.events&EPOLLHUP)
	{
		epoll_ctl(epollFd, EPOLL_CTL_DEL, event.data.fd, NULL);
		return false;
	}
	
	int& fd=event.data.fd;
	if(fd==socket)
	{
		sockaddr_un addr;
		socklen_t addrlen=sizeof(sockaddr_un);
		fd=accept(fd, (sockaddr*)&addr, &addrlen);
		fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL)|O_NONBLOCK)^O_NONBLOCK);
		event.events=EPOLLIN;
		epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
		Message& messageBuffer=fdBuffers[fd].messageBuffer;
		messageBuffer.size=0;
		messageBuffer.type=0;
	}
	Message& messageBuffer=fdBuffers[fd].messageBuffer;
	Header& headerBuffer=fdBuffers[fd].headerBuffer;

	ssize_t actual;
	// Are we in the process of recieving some part of a frame?
	if(!messageBuffer.data)
	{
		// Are we recieving a partial header or new?
		actual=read(fd, (char*)&headerBuffer+messageBuffer.size, sizeof(Header)-messageBuffer.size);
		if(actual<0 && errno!=EAGAIN) throw Exceptions::FastcgiException("Error readin from file descriptor");
		if(actual>0) messageBuffer.size+=actual;
		if(messageBuffer.size!=sizeof(Header))
		{
			if(transmitEmpty) return true;
			else return false;
		}

		messageBuffer.data.reset(new char[sizeof(Header)+headerBuffer.getContentLength()+headerBuffer.getPaddingLength()]);
		memcpy(static_cast<void*>(messageBuffer.data.get()), static_cast<const void*>(&headerBuffer), sizeof(Header));
	}

	const Header& header=*(const Header*)messageBuffer.data.get();
	size_t needed=header.getContentLength()+header.getPaddingLength()+sizeof(Header)-messageBuffer.size;
	actual=read(fd, messageBuffer.data.get()+messageBuffer.size, needed);
	if(actual<0 && errno!=EAGAIN) throw Exceptions::FastcgiException("Error readin from file descriptor");
	if(actual>0) messageBuffer.size+=actual;

	// Did we recieve a full frame?
	if(actual==needed)
	{
		sendMessage(FullId(headerBuffer.getRequestId(), fd), messageBuffer);
		messageBuffer.size=0;
		messageBuffer.data.reset();
		return false;
	}
	if(transmitEmpty) return true;
	else return false;
}

void Fastcgipp::Transceiver::Buffer::freeRead(size_t size)
{
	pRead+=size;
	if(pRead>=chunks.begin()->end)
	{
		if(writeIt==chunks.begin())
		{
			pRead=writeIt->data.get();
			writeIt->end=pRead;
		}
		else
		{
			if(writeIt==--chunks.end())
			{
				chunks.begin()->end=chunks.begin()->data.get();
				chunks.splice(chunks.end(), chunks, chunks.begin());
			}
			else
				chunks.pop_front();
			pRead=chunks.begin()->data.get();
		}
	}
	if((frames.front().size-=size)==0)
	{
		if(frames.front().closeFd)
		{
			epoll_ctl(epollFd, EPOLL_CTL_DEL, frames.front().id.fd, NULL);
			close(frames.front().id.fd);
			fdBuffers.erase(frames.front().id.fd);
		}
		frames.pop();
	}

}

void Fastcgipp::Transceiver::sleep()
{
	sigset_t sigSet;
	sigemptyset(&sigSet);
	sigprocmask(SIG_BLOCK, NULL, &sigSet);
	sigdelset(&sigSet, SIGUSR2);
	epoll_event event;
	epoll_pwait(epollFd, &event, 1, -1, &sigSet);
}

Fastcgipp::Transceiver::Transceiver(int fd_, boost::function<void(Protocol::FullId, Message)> sendMessage_): socket(fd_), sendMessage(sendMessage_), epollFd(epoll_create(16)), buffer(epollFd, fdBuffers)
{
	fcntl(socket, F_SETFL, (fcntl(socket, F_GETFL)|O_NONBLOCK)^O_NONBLOCK);
	epoll_event event;
	event.events=EPOLLIN;
	event.data.fd=socket;
	epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &event);
}
