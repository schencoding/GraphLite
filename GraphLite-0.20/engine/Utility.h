/**
 * @file Utility.h
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
 * This file defines constants and message types.
 *
 */

#ifndef UTILITY_H
#define UTILITY_H

#include "Addr.h"

#define SPRINTF_LEN 1000  // used for sprintf() buffer
// #define MEMPOOL_CAPACITY 1<<8 // memory pool capacity in ChunkedList&FreeList
#define MEMPOOL_CAPACITY 1<<20   // memory pool capacity in ChunkedList&FreeList
// #define SENDLIST_LEN 2 // length of sendlist in Worker, in message
#define SENDLIST_LEN 10000  // length of sendlist in Worker, in message
#define BUFFER_SIZE 1000000 // size of Sender/Receiver buffer, in bytes
/*
 * There should be SENDLIST_LEN*Msg::m_size<=BUFFER_SIZE, since for pack(),
 * SENDLIST_LEN*Msg::m_size is the message length before packed, BUFFER_SIZE
 * should be larger than packed length, and packed length must be less than
 * the length before packed, so SENDLIST_LEN*Msg::m_size<=BUFFER_SIZE.
 */

/** A enum for in-message deliver method. */
typedef enum InMsgDeliverMethod {
    IMDM_OPT_PLAIN,
    IMDM_OPT_GROUP_PREF,
    IMDM_OPT_SWPL_PREF
} IMDM;

/** A enum for message type, only defined member but no instance. */
enum MessageType {
    WM_BEGIN,       /**< worker requests for whole supersteps begin */
    MW_BEGIN,       /**< master responds to whole supersteps begin */
    WM_CURSSFINISH, /**< worker current superstep finishes */
    MW_NEXTSSSTART, /**< master next superstep starts */
    MW_END,         /**< master notifies workers to end supersteps */
    WM_END,         /**< worker reports to master after ending supersteps */
    WW_NODEMSGLIST  /**< worker Node2Node message list */
/*
 * Notice there's no message of WW_FINISHSENDNODEMSG kind, cuz it is
 * WW_NODEMSGLIST with num_msgs=0. We just use this expression to illustrate
 * in annotation.
 */
};

#endif
