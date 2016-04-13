/**
 * @file Receiver.h
 * @author  Songjie Niu, Shimin Chen
 * @version 0.1
 *
 * @section LICENSE 
 * 
 * Copyright 2016 Shimin Chen (chensm@ict.ac.cn) and
 * Songjie Niu (niusongjie@ict.ac.cn)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @section DESCRIPTION
 * 
 * This file defined Receiver class used to receive messages between machines.

 * We temporarily choose TCP/IP as communication protocol through socket,
 * and later we may change protocol into UDP to improve performance.

 * In receive-thread, Receiver keeps on receiving all the time if there are
 * empty buffers. In order to receive efficiently, we use select() non-blocking
 * model and set MSG_DONTWAIT flag in recv(). When all buffers are full,
 * Receiver signal() main thread, and wait() until signaled by main, that is,
 * when there is at least one empty buffer.
 *
 * @see MsgBuffer class
 *
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "MsgBuffer.h"
#include "Utility.h"

extern int main_term;

/** Definition of Sender class. */
class Receiver {
public:
    int m_mysock_fd;     /**< server self socket file descriptor */
    int m_cli_cnt;       /**< count of clients */
    int* m_sock_fd;      /**< sockets with all clients */
    int m_max_sock;      /**< maximum socket in m_sock_fd, used for select() */
    struct sockaddr_in* m_cli_addr; /**< address of clients */
    fd_set m_fds;           /**< file descriptor set for all sockets */
    MsgBuffer* m_in_buffer; /**< in buffers for all clients */

    pthread_mutex_t m_in_mutex; /**< mutex for m_in_buffer */

public:
    /**
     * Initialize.
     * @param cnt client count
     */
    void init(int cnt);

    /**
     * Bind server name to got socket.
     * @param port server port
     */
    void bindServerAddr(int port);

    /** Listen to clients. */
    void listenClient();

    /** Accept clients. */
    void acceptClient();

    /** Receive messages from all clients continuously. */
    void recvMsg();

    /** Close sockets with all clients. */
    void closeAllSocket();
}; // definition of Receiver class

#endif /* RECEIVER_H */
