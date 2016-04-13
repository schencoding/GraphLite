/**
 * @file Master.h
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
 * This file defined Master class to coordinate workers and control progress
 * of supersteps.
 *
 */

#ifndef MASTER_H
#define MASTER_H

#include <pthread.h>
#include <string>

#include "Utility.h"
#include "Graph.h"
#include "AggregatorBase.h"
#include "Sender.h"
#include "Receiver.h"
#include "WM.begin.pb-c.h"
#include "MW.begin.pb-c.h"
#include "WM.curss_finish.pb-c.h"
#include "MW.nextss_start.pb-c.h"
#include "MW.end.pb-c.h"
#include "WM.end.pb-c.h"

extern int main_term;

/** Definition of Master class. */
class Master {
public:
    const char* m_pstart_file; /**< start worker script file path */
    const char* m_puser_file;  /**< user file path */
    std::string m_algo_args;   /**< algorithm arguments */
    IMDM        m_imdm;        /**< in-message deliver method */
    void* m_puf_handle;        /**< handle of opened user file */
    Graph* m_pmy_graph;        /**< configuration class */
    int m_machine_cnt;         /**< machine count, one master and some workers */
    Addr* m_paddr_table;       /**< address table, master 0 workers from 1 */
    Addr m_addr_self;          /**< self address */
    int m_hdfs_flag;           /**< read input from hdfs or local-fs, hdfs 1 local-fs 0 */
    int m_my_aggregator_cnt;           /**< aggregator count */
    AggregatorBase** m_pmy_aggregator; /**< pointers of AggregatorBase */

    Sender    m_sender;      /**< to manage activities about send */
    Receiver  m_receiver;    /**< to manage activities about receive */
    pthread_t m_pth_send;    /**< send thread */
    pthread_t m_pth_receive; /**< receive thread */

    Wm__Begin*       m_pwm_begin;       /**< worker requests for whole supersteps begin */
    Mw__Begin        m_mw_begin;        /**< master responds to whole supersteps begin */
    Wm__CurssFinish* m_pwm_curssfinish; /**< worker current superstep finishes */
    Mw__NextssStart  m_mw_nextssstart;  /**< master next superstep starts */
    Mw__End          m_mw_end;          /**< master notifies workers to end supersteps */
    Wm__End*         m_pwm_end;         /**< worker reports to master after ending supersteps */

    int m_term;            /**< to mark if supersteps end, 1/0 yes/no */
    int* m_pfinish_send;   /**< to mark if MW_* message sent successfully to every worker, from [1] */
    int m_ready2begin_wk;  /**< count of workers ready to begin */
    int m_curssfinish_wk;  /**< count of workers having finished current superstep */
    int64_t* m_worker_msg; /**< count of messages for each worker in current superstep */
    int64_t m_act_vertex;  /**< count of active vertices left after current superstep */
    int64_t m_sent_msg;    /**< count of messages sent in current superstep */
    int m_alreadyend_wk;   /**< count of workers already ends */

public:
    /**
     * Run function.
     * Master process entrance, which consists of child methods.
     * @param argc command line argument number
     * @param argv command line arguments
     */
    void run(int argc, char* argv[]);

    /**
     * Parse command line arguments.
     * @param argc command line argument number
     * @param argv command line arguments
     */
    void parseCmdArg(int argc, char* argv[]);

    /**
     * Load user file.
     * @param argc command line argument number
     * @param argv command line arguments
     */
    void loadUserFile(int argc, char* argv[]);

    /** Start all worker processes. */
    void startWorkers(char* argv[]);

    /** Initialize some global/member variables. */
    void init();

    /**
     * Respond whole supersteps begin message to a worker.
     * @param worker_id destination worker id
     * @retval 0 send successfully
     * @retval 1 send unsuccessfully
     */
    int sendBegin(int worker_id);

    /**
     * Send next superstep start message to a worker.
     * @param worker_id destination worker id
     * @retval 0 send successfully
     * @retval 1 send unsuccessfully
     */
    int sendNextssstart(int worker_id);

    /**
     * Send whole superstep end message to a worker.
     * @param worker_id destination worker id
     * @retval 0 send successfully
     * @retval 1 send unsuccessfully
     */
    int sendEnd(int worker_id);

    /**
     * Send messages of same type to all workers.
     * @param msg_type message type
     */
    void sendAll(int msg_type);

    /**
     * Receive all kinds of messages from a worker.
     * @param worker_id source worker id
     */
    void receiveMessage(int worker_id);

    /** Manage a series of supersteps. */
    void manageSuperstep();

    /** Free some global/member variables. */
    void terminate();
}; // definition of Master class

#endif /* MASTER_H */
