/**
 * @file Sender.h
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
 * This file defined Sender class used to send messages between machines.

 * We temporarily choose TCP/IP as communication protocol through socket,
 * and later we may change protocol into UDP to improve performance.

 * In send-thread, Sender keeps on sending all the time if there are messages
 * in buffers. In order to send efficiently, we use select() non-blocking model
 * and set MSG_DONTWAIT flag in send(). When all buffers are empty, Sender
 * signal() main thread, and wait() until signaled by main, that is, when there
 * is at least one full buffer.
 *
 * @see MsgBuffer class
 *
 */

#ifndef SENDER_H
#define SENDER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "MsgBuffer.h"
#include "Utility.h"

extern int main_term;

/** Definition of Sender class. */
class Sender {
public:
    int m_serv_cnt;      /**< count of servers */
    int* m_sock_fd;      /**< sockets with all servers */
    int m_max_sock;      /**< maximum socket in m_sock_fd, used for select() */
    struct sockaddr_in* m_serv_addr; /**< address of servers */
    fd_set m_fds;            /**< file descriptor set for all sockets */
    MsgBuffer* m_out_buffer; /**< out buffers for all servers */

    pthread_cond_t  m_out_cond;  /**< condition variable for m_out_buffer */
    pthread_mutex_t m_out_mutex; /**< mutex for m_out_buffer */

public:
    /**
     * Initialize.
     * @param cnt server count
     */
    void init(int cnt);

    /** Get sockets for all servers. */
    void getSocketFd();

    /**
     * Get server addresses.
     * @param addr server address table
     */
    void getServerAddr(Addr* addr);

    /**
     * Build connection with all servers.
     * @param id self machine id
     */
    void connectServer(int id);

    /** Send messages to all servers continuously. */
    void sendMsg();

    /** Close sockets with all servers. */
    void closeAllSocket();
}; // definition of Sender class

#endif /* SENDER_H */
