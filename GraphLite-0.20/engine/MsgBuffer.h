/**
 * @file MsgBuffer.h
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
 * This file defined MsgBuffer class used to store network messages.
 * We define out_buffer of MsgBuffer type in Sender class to store messages to
 * sent, and in_buffer of MsgBuffer type in Receiver class to store messages to
 * receive.
 *
 * A whole network message includes buf_len, msg_type, and packed messages in
 * buffer. In detail, buf_len is the length of a whole network message in bytes,
 * including occupied bytes of buf_len, msg_type and packed messages; msg_len is
 * the length of a network message content in bytes, including occupied bytes of
 * msg_type and packed messages.
 * In case below, m_buffer first sizeof(int) bytes to store buf_len value,
 * second sizeof(int) bytes to store msg_type value, and rest bytes to store
 * packed messages from m_buffer[2 * sizeof(int)].
 *
 * In order to deal with the situation where one piece of network message hasn't
 * been sent or received totally in one send() or recv(), we use m_head & m_tail
 * to mark positions. In send-thread, m_head suggests head position of
 * the left-to-send message, and accumulates return value of send(); in
 * receive-thread, m_tail suggests tail position of the already-received
 * message, more precisely, not tail but the next position of tail, and
 * accumulates return value of recv().
 * m_head plays no role in receive-thread, and same as m_tail in send-thread.
 *
 */

#ifndef MSGBUFFER_H
#define MSGBUFFER_H

#include "Utility.h"

/** Definition of MsgBuffer class. */
class MsgBuffer {
public:
    int m_state; /**< indicate buffer state-
                  0: for sender, totally empty; for receiver, totally/partly empty.
                  1: for sender, totally/partly full; for receiver, totally full. */
    int m_head; /**< mark head position of left-to-send message */
    int m_tail; /**< mark tail position of already-received message */     
    int m_buf_len;                /**< length of whole network message */
    int m_msg_type;               /**< network message type */
    int m_msg_len;                /**< length of network message content */
    uint8_t m_buffer[BUFFER_SIZE]; /**< network message buffer */

public:
    MsgBuffer(): m_state(0), m_head(0), m_tail(0), m_buf_len(0), m_msg_type(0),
                 m_msg_len(0) {}
}; // definition of MsgBuffer class

#endif /* MSGBUFFER_H */
