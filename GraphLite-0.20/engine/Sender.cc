/**
 * @file Sender.cc
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
 * @see Sender.h
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "Sender.h"

void Sender::init(int cnt) {

    // 1. Set server count.
    m_serv_cnt = cnt;

    // 2. Get out_buffer memory.
    m_out_buffer = new MsgBuffer[m_serv_cnt];
    if (! m_out_buffer) {
        perror("Sender: new");
        exit(1);
    }

    // 3. Initialize out_mutex.
    m_out_mutex = PTHREAD_MUTEX_INITIALIZER;
}

void Sender::getSocketFd() {
    m_sock_fd = (int *)malloc( m_serv_cnt * sizeof(int) );
    if (! m_sock_fd) {
        perror("Sender: malloc");
        exit(1);
    }

    m_max_sock = 0;
    for (int i = 0; i < m_serv_cnt; ++i) {
        m_sock_fd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sock_fd[i] < 0) {
            perror("Sender: socket");
            exit(1);
        }

        if (m_sock_fd[i] > m_max_sock) {
            m_max_sock = m_sock_fd[i];
        }
    }
}

void Sender::getServerAddr(Addr *addr) {
    m_serv_addr = (struct sockaddr_in *)malloc( m_serv_cnt * sizeof(struct sockaddr_in) );
    if (! m_serv_addr) {
        perror("Sender: malloc");
        exit(1);
    }

    for (int i = 0; i < m_serv_cnt; ++i) {
        memset( (char *) &m_serv_addr[i], 0, sizeof(struct sockaddr_in) );
        m_serv_addr[i].sin_family = AF_INET;
        struct hostent * server = gethostbyname(addr[i].hostname);
        if (!server) {
            perror("Sender: no host");
            exit(1);
        }

        memcpy( (char *)&m_serv_addr[i].sin_addr.s_addr, (char *)server->h_addr, server->h_length );
        m_serv_addr[i].sin_port = htons(addr[i].port);
    }
}

void Sender::connectServer(int id) {
    char buffer[SPRINTF_LEN];

    for (int i = 0; i < m_serv_cnt; ++i) {
        int retries = 0;

        while (connect( m_sock_fd[i], (struct sockaddr *)&m_serv_addr[i], sizeof(struct sockaddr_in) ) < 0) {
            if (errno != ECONNREFUSED) {
                perror("Sender: connect");
                exit(1);
            }

            ++retries;
            if (retries % 10 == 0) {
                perror("Sender: connect");
                if (retries >= 60) {
                  fprintf(stderr, "Sender cannot connect after %d retries\n", 
                          retries);
                  exit(1);
                }
            }

            sleep(1);
        }

        memset( buffer, 0, sizeof(buffer) );
        sprintf(buffer, "%d", id);
        send(m_sock_fd[i], buffer, sizeof(buffer), 0);
    }

    printf("Sender: connect all server success\n"); fflush(stdout);
}

void Sender::sendMsg() {
    fd_set fds_orig;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds_orig);
    for (int i = 0; i < m_serv_cnt; ++i) {
        FD_SET(m_sock_fd[i], &fds_orig);
    }

    // int loop = 0;
    while (! main_term) {
        // ++loop;
        // printf("Sender: loop %d\n", loop); fflush(stdout);

        FD_ZERO(&m_fds);
        m_fds = fds_orig;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(m_max_sock + 1, NULL, &m_fds, NULL, &tv); // writable
        // printf("Sender: select ret %d\n", ret); fflush(stdout);
        if (ret < 0) {
            perror("Sender: select");
            break;
        } else if (!ret) {
            printf("Sender: timeout\n"); fflush(stdout);
            continue;
        }

        for (int i = 0; i < m_serv_cnt; ++i) {
            if (m_out_buffer[i].m_state) { // At least one buffer has data.
                if ( FD_ISSET(m_sock_fd[i], &m_fds) ) { // Socket i has been set.
                    // send
                    // printf("Sender: send()\n"); fflush(stdout);
                    ret = send(m_sock_fd[i], m_out_buffer[i].m_buffer + m_out_buffer[i].m_head,
                               m_out_buffer[i].m_buf_len, MSG_DONTWAIT);
                    // printf("Sender: ret %d\n", ret); fflush(stdout);

                    pthread_mutex_lock(&m_out_mutex);
                    if (ret < m_out_buffer[i].m_buf_len) {
                        m_out_buffer[i].m_head += ret;
                        m_out_buffer[i].m_buf_len -= ret;
                    } else if (ret == m_out_buffer[i].m_buf_len) {
                        m_out_buffer[i].m_state = 0;
                        m_out_buffer[i].m_head = 0;
                        m_out_buffer[i].m_tail = 0;
                        m_out_buffer[i].m_msg_len = 0;
                        m_out_buffer[i].m_buf_len = 0;
                        // memset m_out_buffer[i].m_buffer
                    }
                    pthread_cond_signal(&m_out_cond);
                    pthread_mutex_unlock(&m_out_mutex);
                }
            }
        }
    }

    // printf("Sender: break select\n"); fflush(stdout);
}

void Sender::closeAllSocket() {

    // 1. Close all socket.
    for (int i = 0; i < m_serv_cnt; ++i) {
        close(m_sock_fd[i]);
    }

    // 2. Free memory allocated.
    free(m_serv_addr);
    free(m_sock_fd);
    delete[] m_out_buffer;

    printf("Sender: closeAllSocket\n"); fflush(stdout);
}
