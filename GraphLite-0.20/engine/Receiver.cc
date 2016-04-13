/**
 * @file Receiver.cc
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
 * @see Receiver.h
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "Receiver.h"

void Receiver::init(int cnt) {

    // 1. Set client count.
    m_cli_cnt = cnt;

    // 2. Get in_buffer memory.
    m_in_buffer = new MsgBuffer[m_cli_cnt];
    if (! m_in_buffer) {
        perror("Receiver: new");
        exit(1);
    }

    // 3. Initialize in_mutex.
    m_in_mutex = PTHREAD_MUTEX_INITIALIZER;

    // 4. Get server self socket.
    m_mysock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_mysock_fd < 0) {
        perror("Receiver: socket");
        exit(1);
    }
}

void Receiver::bindServerAddr(int port) {
    // Shimin mod: set sockopt
    int optval= 1;
    setsockopt(m_mysock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in cli_addr;
    memset( (char *) &cli_addr, 0, sizeof(cli_addr) );
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    cli_addr.sin_port = htons(port);

    int retries = 0;
    while (bind( m_mysock_fd, (struct sockaddr *) &cli_addr, sizeof(cli_addr) ) < 0) {
        if (errno != EADDRINUSE) {
           perror("Receiver: bind");
           exit(1);
        }

        ++retries;
        if (retries % 10 == 0) {
           perror("Receiver: bind");
        }

        sleep(1);
    }
}

void Receiver::listenClient() {
    listen(m_mysock_fd, m_cli_cnt);
}

void Receiver::acceptClient() {
    int sock;
    char buffer[SPRINTF_LEN];
    int machine_no;

    m_sock_fd = (int *)malloc( m_cli_cnt * sizeof(int) );
    if (! m_sock_fd) {
        perror("Receiver: malloc");
        exit(1);
    }

    m_max_sock = 0;
    m_cli_addr = (struct sockaddr_in *)malloc( m_cli_cnt * sizeof(struct sockaddr_in) );
    if (! m_cli_addr) {
        perror("Receiver: malloc");
        exit(1);
    }

    int clilen = sizeof(struct sockaddr_in);
    for (int i = 0; i < m_cli_cnt; ++i) {
        sock = accept(m_mysock_fd, (struct sockaddr *)&m_cli_addr[i], (socklen_t*)&clilen);
        if (sock < 0) {
            perror("Receiver: accept");
            continue;
        }
        if (sock > m_max_sock) {
            m_max_sock = sock;
        }

        memset( buffer, 0, sizeof(buffer) );
        recv(sock, buffer, sizeof(buffer), 0);
        machine_no = atoi(buffer);
        m_sock_fd[machine_no] = sock;
        
    }

    printf("Receiver: accept all client success\n"); fflush(stdout);
}

void Receiver::recvMsg() {
    fd_set fds_orig;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds_orig);
    for (int i = 0; i < m_cli_cnt; ++i) {
        FD_SET(m_sock_fd[i], &fds_orig);
    }

    // int loop = 0;
    int retries = 0;
    while (! main_term) {
        // ++loop;
        // printf("Receiver: loop %d\n", loop); fflush(stdout);

        FD_ZERO(&m_fds);
        m_fds = fds_orig;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(m_max_sock + 1, &m_fds, NULL, NULL, &tv); // readable
        // printf("Receiver: select ret %d\n", ret); fflush(stdout);
        if (ret < 0) {
            perror("Receiver: select");
            break;
        } else if (!ret) {
            ++retries;
            if (retries % 100 == 0) {
               printf("Receiver: timeout\n"); fflush(stdout);
            }

            sleep(1);
            continue;
        }

        for (int i = 0; i < m_cli_cnt; ++i) {
            if (! m_in_buffer[i].m_state) { // At least one buffer doesn't have complete data.
                if ( FD_ISSET(m_sock_fd[i], &m_fds) ) { // Socket i has been set.

                    // get buf_len remained
                    pthread_mutex_lock(&m_in_mutex);
                    int buf_len = m_in_buffer[i].m_buf_len;
                    pthread_mutex_unlock(&m_in_mutex);

                    // Every message needs to call recv() at least twice, first for message length and rest for the content.
                    if (! buf_len) { // buf_len hasn't been read in completely.
                        // receive
                        // printf("Receiver: buf_len recv()\n"); fflush(stdout);
                        ret = recv(m_sock_fd[i], &m_in_buffer[i].m_buffer[m_in_buffer[i].m_tail],
                                   sizeof(m_in_buffer[i].m_buf_len) - m_in_buffer[i].m_tail, MSG_DONTWAIT);
                        // printf("Receiver: buf_len ret %d\n", ret); fflush(stdout);
                        if (ret == 0) { // When peer client socket close, server receive ret = 0, necessary.
                            FD_CLR(m_sock_fd[i], &fds_orig);
                            continue;
                        }

                        pthread_mutex_lock(&m_in_mutex);
                        m_in_buffer[i].m_tail += ret;
                        if ( m_in_buffer[i].m_tail == sizeof(m_in_buffer[i].m_buf_len) ) { // buf_len has been read in completely.
                            m_in_buffer[i].m_buf_len = * (int *)m_in_buffer[i].m_buffer;
                            m_in_buffer[i].m_msg_len = m_in_buffer[i].m_buf_len - sizeof(int);
                        }
                        pthread_mutex_unlock(&m_in_mutex);
                    } else { // buf_len has been read in completely.
                        // receive
                        // printf("Receiver: recv()\n"); fflush(stdout);
                        ret = recv(m_sock_fd[i], &m_in_buffer[i].m_buffer[m_in_buffer[i].m_tail],
                                   m_in_buffer[i].m_msg_len, MSG_DONTWAIT);
                        // printf("Receiver: ret %d\n", ret); fflush(stdout);
                        if (ret == 0) { // When peer client socket close, server receive ret = 0, necessary.
                            FD_CLR(m_sock_fd[i], &fds_orig);
                            continue;
                        }

                        pthread_mutex_lock(&m_in_mutex);
                        if (ret < m_in_buffer[i].m_msg_len) {
                            m_in_buffer[i].m_tail += ret;
                            m_in_buffer[i].m_msg_len -= ret;
                        } else if (ret == m_in_buffer[i].m_msg_len) {
                            m_in_buffer[i].m_state = 1;
                            m_in_buffer[i].m_head = 0;
                            m_in_buffer[i].m_tail = 0;
                            m_in_buffer[i].m_msg_len = 0;
                            m_in_buffer[i].m_buf_len = 0;
                            // memset m_out_buffer[i].m_buffer
                        }
                        pthread_mutex_unlock(&m_in_mutex);
                    }
                }
            }
        }
    }

    // printf("Receiver: break select\n"); fflush(stdout);
}

void Receiver::closeAllSocket() {

    // 1. Close all socket.
    for (int i = 0; i < m_cli_cnt; ++i) {
        close(m_sock_fd[i]);
    }

    // 2. Free memory allocated.
    free(m_cli_addr);
    free(m_sock_fd);
    delete[] m_in_buffer;

    printf("Receiver: closeAllSocket\n"); fflush(stdout);
}
