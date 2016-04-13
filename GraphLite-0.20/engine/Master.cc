#include <dlfcn.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "Master.h"

extern Master master;

struct timeval b_time, e_time;
double elapsed = 0;

void* m_receiveFun(void *args) {
    master.m_receiver.bindServerAddr(master.m_addr_self.port);
    master.m_receiver.listenClient();
    master.m_receiver.acceptClient();
    master.m_receiver.recvMsg();
    master.m_receiver.closeAllSocket();
    return NULL;
}

void* m_sendFun(void *args) {
    master.m_sender.getSocketFd();
    master.m_sender.getServerAddr(master.m_paddr_table);
    master.m_sender.connectServer(master.m_addr_self.id);
    master.m_sender.sendMsg();
    master.m_sender.closeAllSocket();
    return NULL;
}

void Master::run(int argc, char* argv[]) {
    parseCmdArg(argc, argv);
    loadUserFile(argc, argv);
    startWorkers(argv);
    init();
    manageSuperstep();
    terminate();
}

void Master::parseCmdArg(int argc, char* argv[]) {
    printf("parseCmdArg\n"); fflush(stdout);

    m_addr_self.id = atoi(argv[1]);
    m_pstart_file = argv[2];
    m_puser_file = argv[3];
    for (int i = 4; i < argc - 1; ++i) {
        m_algo_args += argv[i];
        m_algo_args += " ";
    }
    if (argc > 4) {
        m_algo_args += argv[argc - 1];
    }
    // Parsing user cmd line is below in Graph init().
    int imdm = 2;
    switch (imdm) {
    case 0:
        m_imdm = IMDM_OPT_PLAIN;
        break;
    case 1:
        m_imdm = IMDM_OPT_GROUP_PREF;
        break;
    case 2:
        m_imdm = IMDM_OPT_SWPL_PREF;
        break;
    default:
        break;
    }
}

void Master::loadUserFile(int argc, char* argv[]) {
    printf("loadUserFile\n"); fflush(stdout);

    // 1. Open user file.
    m_puf_handle = dlopen(m_puser_file, RTLD_NOW);
    if (! m_puf_handle) {
       fprintf( stderr, "%s\n", dlerror() );
       exit(1);
    }

    // 2. Create Graph class.
    GraphCreateFn create_graph;
    create_graph = (GraphCreateFn)dlsym(m_puf_handle, "create_graph");
    m_pmy_graph = create_graph();
    m_pmy_graph->init(argc - 3, (char **)&argv[3]);

    // 3. Read in configuration from Graph.
    m_machine_cnt = m_pmy_graph->m_machine_cnt;
    m_paddr_table = m_pmy_graph->m_paddr_table;
    m_hdfs_flag = m_pmy_graph->m_hdfs_flag;
    m_my_aggregator_cnt = m_pmy_graph->m_aggregator_cnt;
    m_pmy_aggregator = m_pmy_graph->m_paggregator;
}

void Master::startWorkers(char* argv[]) {
    printf("startWorkers\n"); fflush(stdout);

    char *curdir= get_current_dir_name(); // we cd to the curdir then call start-worker

    char s[SPRINTF_LEN];
    int ret;
    for (int i = 1; i < m_machine_cnt; ++i) {
	    if (m_algo_args == "") {
            /* ssh command, no srcipt for .profile
            sprintf(s, "ssh %s '%s %d %s > worker%d.out 2>&1 </dev/null &'",
            m_paddr_table[i].hostname, argv[0], m_paddr_table[i].id, m_puser_file, m_paddr_table[i].id);
            */
            sprintf(s, "ssh %s 'cd %s; %s %s %d %s %d'", m_paddr_table[i].hostname, curdir,
            m_pstart_file, argv[0], m_paddr_table[i].id, m_puser_file, m_paddr_table[i].id);
	     } else {
            /* ssh command, no srcipt for .profile
            sprintf(s, "ssh %s '%s %d %s %s > worker%d.out 2>&1 </dev/null &'",
            m_paddr_table[i].hostname, argv[0], m_paddr_table[i].id, m_puser_file, m_algo_args.c_str(), m_paddr_table[i].id);
            */
            sprintf(s, "ssh %s 'cd %s; %s %s %d %s %s %d'", m_paddr_table[i].hostname, curdir,
            m_pstart_file, argv[0], m_paddr_table[i].id, m_puser_file, m_algo_args.c_str(), m_paddr_table[i].id);
		 }

        printf("worker %d cmd: %s\n", i, s); fflush(stdout); //

        ///*
        ret = system(s);
        if ( (ret < 0) || (WEXITSTATUS(ret) != 0) ) {
            fprintf(stderr, "failed!\n");
            exit(1);
        }
        //*/
    }

    free(curdir);
}

void Master::init() {
    printf("init\n"); fflush(stdout);

    // 1. Get self address from address table through id.
    strcpy(m_addr_self.hostname, m_paddr_table[m_addr_self.id].hostname);
    m_addr_self.port = m_paddr_table[m_addr_self.id].port;

    // 2. Initialize messages with workers.
    m_pwm_begin = NULL;
    mw__begin__init(&m_mw_begin);
    m_pwm_curssfinish = NULL;
    mw__nextss_start__init(&m_mw_nextssstart);
    m_mw_nextssstart.n_aggr_global = m_my_aggregator_cnt;
    m_mw_nextssstart.aggr_global = (ProtobufCBinaryData *)malloc(
    (m_my_aggregator_cnt < 1 ? 1 : m_my_aggregator_cnt) * sizeof(ProtobufCBinaryData) );
    if (! m_mw_nextssstart.aggr_global) {
        perror("malloc");
        exit(1);
    }
    for (size_t i = 0; i < m_mw_nextssstart.n_aggr_global; ++i) {
        m_mw_nextssstart.aggr_global[i].len = m_pmy_aggregator[i]->getSize();
    }
    mw__end__init(&m_mw_end);
    m_pwm_end = NULL;

    m_pfinish_send = (int *)malloc( m_machine_cnt * sizeof(int) );
    if (! m_pfinish_send) {
        perror("malloc");
        exit(1);
    }

    m_worker_msg = (int64_t *)malloc( m_machine_cnt * sizeof(int64_t) );
    if (! m_worker_msg) {
        perror("malloc");
        exit(1);
    }

    // 3. Initialize sender and receiver.
    m_receiver.init(m_machine_cnt);
    m_sender.init(m_machine_cnt);

    // 4. Create send and receive thread.
    if ( pthread_create(&m_pth_receive, NULL, m_receiveFun, NULL) ) {
        fprintf(stderr, "Error creating receive thread !\n");
        exit(1);
    }
    if ( pthread_create(&m_pth_send, NULL, m_sendFun, NULL) ) {
        fprintf(stderr, "Error creating send thread !\n");
        exit(1);
    }
}

int Master::sendBegin(int worker_id) {

    // 1. Set message content to be sent.
    m_mw_begin.s_id = 0;
    m_mw_begin.d_id = worker_id;
    m_mw_begin.state = 0;

    // 2. Send to destination worker.
    if (! m_sender.m_out_buffer[worker_id].m_state) { // just test, totally empty, can write to m_out_buffer

        pthread_mutex_lock(&m_sender.m_out_mutex);
        if (! m_sender.m_out_buffer[worker_id].m_state) { // totally empty, can write to m_out_buffer
            m_sender.m_out_buffer[worker_id].m_msg_len = sizeof(int) +
                   mw__begin__pack(&m_mw_begin,
                   &m_sender.m_out_buffer[worker_id].m_buffer[2 * sizeof(int)]);
            m_sender.m_out_buffer[worker_id].m_buf_len = m_sender.m_out_buffer[worker_id].m_msg_len + sizeof(int);
            * (int *)m_sender.m_out_buffer[worker_id].m_buffer = m_sender.m_out_buffer[worker_id].m_buf_len;
            * (int *)&(m_sender.m_out_buffer[worker_id].m_buffer[sizeof(int)]) = MW_BEGIN;
            m_sender.m_out_buffer[worker_id].m_head = 0;
            m_sender.m_out_buffer[worker_id].m_tail = m_sender.m_out_buffer[worker_id].m_buf_len;
            m_sender.m_out_buffer[worker_id].m_state = 1;
        }
        pthread_mutex_unlock(&m_sender.m_out_mutex);

        return 0;
    } // else can't write to m_out_buffer

    return 1;
}

int Master::sendNextssstart(int worker_id) {

    // 1. Set message content to be sent.
    m_mw_nextssstart.s_id = 0;
    m_mw_nextssstart.d_id = worker_id;
    m_mw_nextssstart.node_msg = m_worker_msg[worker_id];
    for (size_t i = 0; i < m_mw_nextssstart.n_aggr_global; ++i) {
        m_mw_nextssstart.aggr_global[i].data = (uint8_t *)( m_pmy_aggregator[i]->getGlobal() );
    }

    // 2. Send to destination worker.
    if (! m_sender.m_out_buffer[worker_id].m_state) { // just test, totally empty, can write to m_out_buffer

        pthread_mutex_lock(&m_sender.m_out_mutex);
        if (! m_sender.m_out_buffer[worker_id].m_state) { // totally empty, can write to m_out_buffer
            m_sender.m_out_buffer[worker_id].m_msg_len = sizeof(int) +
                   mw__nextss_start__pack(&m_mw_nextssstart,
                   &m_sender.m_out_buffer[worker_id].m_buffer[2 * sizeof(int)]);
            m_sender.m_out_buffer[worker_id].m_buf_len = m_sender.m_out_buffer[worker_id].m_msg_len + sizeof(int);
            * (int *)m_sender.m_out_buffer[worker_id].m_buffer = m_sender.m_out_buffer[worker_id].m_buf_len;
            * (int *)&(m_sender.m_out_buffer[worker_id].m_buffer[sizeof(int)]) = MW_NEXTSSSTART;
            m_sender.m_out_buffer[worker_id].m_head = 0;
            m_sender.m_out_buffer[worker_id].m_tail = m_sender.m_out_buffer[worker_id].m_buf_len;
            m_sender.m_out_buffer[worker_id].m_state = 1;
        }
        pthread_mutex_unlock(&m_sender.m_out_mutex);

        return 0;
    } // else can't write to m_out_buffer

    return 1;
}

int Master::sendEnd(int worker_id) {

    // 1. Set message content to be sent.
    m_mw_end.s_id = 0;
    m_mw_end.d_id = worker_id;
    m_mw_end.state = 0;

    // 2. Send to destination worker.

    if (! m_sender.m_out_buffer[worker_id].m_state) { // just test, totally empty, can write to m_out_buffer

        pthread_mutex_lock(&m_sender.m_out_mutex);
        if (! m_sender.m_out_buffer[worker_id].m_state) { // totally empty, can write to m_out_buffer
            m_sender.m_out_buffer[worker_id].m_msg_len = sizeof(int) +
                   mw__end__pack(&m_mw_end,
                   &m_sender.m_out_buffer[worker_id].m_buffer[2 * sizeof(int)]);
            m_sender.m_out_buffer[worker_id].m_buf_len = m_sender.m_out_buffer[worker_id].m_msg_len + sizeof(int);
            * (int *)m_sender.m_out_buffer[worker_id].m_buffer = m_sender.m_out_buffer[worker_id].m_buf_len;
            * (int *)&(m_sender.m_out_buffer[worker_id].m_buffer[sizeof(int)]) = MW_END;
            m_sender.m_out_buffer[worker_id].m_head = 0;
            m_sender.m_out_buffer[worker_id].m_tail = m_sender.m_out_buffer[worker_id].m_buf_len;
            m_sender.m_out_buffer[worker_id].m_state = 1;
        }
        pthread_mutex_unlock(&m_sender.m_out_mutex);

        return 0;
    } // else can't write to m_out_buffer

    return 1;
}

void Master::sendAll(int msg_type) {
    printf("step into sendAll\n");

    int msg2send; // count of messages to send
    memset( m_pfinish_send, 0, m_machine_cnt * sizeof(int) );

    switch (msg_type) {
    case MW_BEGIN:
        do {
            msg2send = 0;
            for (int i = 1; i < m_machine_cnt; ++i) {
                if (! m_pfinish_send[i]) {
                    ++msg2send;
                    if ( sendBegin(i) ) continue;
                    printf("sent MW_BEGIN to worker[%d]\n", i); fflush(stdout);
                    m_pfinish_send[i] = 1;
                    --msg2send;
                }
            }
        } while (msg2send);
        break;
    case MW_NEXTSSSTART:
        do {
            msg2send = 0;
            for (int i = 1; i < m_machine_cnt; ++i) {
                if (! m_pfinish_send[i]) {
                    ++msg2send;
                    if ( sendNextssstart(i) ) continue;
                    printf("sent MW_NEXTSSSTART to worker[%d]\n", i); fflush(stdout);
                    m_pfinish_send[i] = 1;
                    --msg2send;
                }
            }
        } while (msg2send);
        break;
    case MW_END:
        do {
            msg2send = 0;
            for (int i = 1; i < m_machine_cnt; ++i) {
                if (! m_pfinish_send[i]) {
                    ++msg2send;
                    if ( sendEnd(i) ) continue;
                    printf("sent MW_END to worker[%d]\n", i); fflush(stdout);
                    m_pfinish_send[i] = 1;
                    --msg2send;
                }
            }
        } while (msg2send);
        break;
    default:
        fprintf(stderr, "There is no such message type !\n");
        break;
    }
}

void Master::receiveMessage(int worker_id) {

    if (m_receiver.m_in_buffer[worker_id].m_state) { // just test, totally full, can read from m_in_buffer

        pthread_mutex_lock(&m_receiver.m_in_mutex);
        if (m_receiver.m_in_buffer[worker_id].m_state) { // totally full, can read from m_in_buffer
            m_receiver.m_in_buffer[worker_id].m_buf_len = * (int *)m_receiver.m_in_buffer[worker_id].m_buffer;
            m_receiver.m_in_buffer[worker_id].m_msg_type = * (int *)&(m_receiver.m_in_buffer[worker_id].m_buffer[sizeof(int)]);

            if (m_receiver.m_in_buffer[worker_id].m_buf_len) {
                m_receiver.m_in_buffer[worker_id].m_msg_len = m_receiver.m_in_buffer[worker_id].m_buf_len - sizeof(int);
                int pack_len = m_receiver.m_in_buffer[worker_id].m_msg_len - sizeof(int);

                 switch (m_receiver.m_in_buffer[worker_id].m_msg_type) {
                 case WM_BEGIN:
                     m_pwm_begin = wm__begin__unpack(NULL, pack_len, &m_receiver.m_in_buffer[worker_id].m_buffer[2 * sizeof(int)]);
                     if (m_pwm_begin->state == 0) { // this worker ready to begin
                         ++m_ready2begin_wk;
                     }
                     wm__begin__free_unpacked(m_pwm_begin, NULL);
                     break;
                 case WM_CURSSFINISH:
                     m_pwm_curssfinish = wm__curss_finish__unpack(NULL, pack_len, &m_receiver.m_in_buffer[worker_id].m_buffer[2 * sizeof(int)]);
                     for (size_t i = 0; i < m_pwm_curssfinish->n_aggr_local; ++i) {
                         m_pmy_aggregator[i]->merge(m_pwm_curssfinish->aggr_local[i].data);
                         // printf( "m_pmy_aggregator[%ld]: %f\n", i, * (double *)m_pmy_aggregator[i]->getGlobal() ); fflush(stdout);
                     }
                     for (size_t i = 0; i < m_pwm_curssfinish->n_worker_msg; ++i) {
                         m_worker_msg[i] += m_pwm_curssfinish->worker_msg[i];
                     }
                     m_act_vertex += m_pwm_curssfinish->act_vertex;
                     m_sent_msg += m_pwm_curssfinish->sent_msg;
                     // printf("m_act_vertex: %ld, m_sent_msg: %ld\n", m_act_vertex, m_sent_msg); fflush(stdout);
                     wm__curss_finish__free_unpacked(m_pwm_curssfinish, NULL);
                     ++m_curssfinish_wk;
                     break;
                 case WM_END:
                     m_pwm_end = wm__end__unpack(NULL, pack_len, &m_receiver.m_in_buffer[worker_id].m_buffer[2 * sizeof(int)]);
                     if (m_pwm_end->state == 0) { // this worker already ends
                         ++m_alreadyend_wk;
                     }
                     wm__end__free_unpacked(m_pwm_end, NULL);
                     break;
                 default:
                     fprintf(stderr, "There is no such message type !\n");
                     break;
                 }

                m_receiver.m_in_buffer[worker_id].m_buf_len = 0;
                m_receiver.m_in_buffer[worker_id].m_state = 0;
            }
        }
        pthread_mutex_unlock(&m_receiver.m_in_mutex);

    } // else can't read from m_in_buffer
}

void Master::manageSuperstep() {
    printf("manageSuperstep\n"); fflush(stdout);

    // 1. Receive worker requests for whole supersteps begin.
    m_ready2begin_wk = 1;
    while (m_ready2begin_wk < m_machine_cnt) {
        for (int i = 1; i < m_machine_cnt; ++i) receiveMessage(i);
    }
    printf("received WM_BEGIN\n"); fflush(stdout);

    // 2. Send responds to whole supersteps begin.
    printf("MW_BEGIN: %d\n", MW_BEGIN); fflush(stdout);
    sendAll(MW_BEGIN);
    printf("sent MW_BEGIN\n"); fflush(stdout);

    // 3. Run into supersteps.
    m_term = 0;
    m_mw_nextssstart.superstep = -1;
    m_alreadyend_wk = 1;

    gettimeofday(&b_time, NULL);

    while (! m_term) {
        // 3.1 Initialize before every superstep.
        ++m_mw_nextssstart.superstep;
        printf("-----------------------------------------\n"); fflush(stdout);
        printf("superstep: %d\n", m_mw_nextssstart.superstep); fflush(stdout);
        for (int i = 0; i < m_my_aggregator_cnt; ++i) m_pmy_aggregator[i]->init();
        m_curssfinish_wk = 1;

        // 3.2 Receive current superstep finish message from workers.
        memset( m_worker_msg, 0, m_machine_cnt * sizeof(int64_t) );
        m_act_vertex = 0;
        m_sent_msg = 0;
        while (m_curssfinish_wk < m_machine_cnt) {
            for (int i = 1; i < m_machine_cnt; ++i) receiveMessage(i);
        }
        printf("received WM_CURSSFINISH\n"); fflush(stdout);

        // 3.3 Check if supersteps terminate.
        if ( ( m_pmy_graph->masterComputePerstep(m_mw_nextssstart.superstep, m_pmy_aggregator) ) ||
             (!m_act_vertex && !m_sent_msg) ) m_term = 1;

        // 3.4 Send terminate/go on supersteps message to workers.
        if (m_term) {
            printf("MW_END: %d\n", MW_END); fflush(stdout);
            sendAll(MW_END);
            printf("sent MW_END\n"); fflush(stdout);

            // receive already end message from workers
            while (m_alreadyend_wk < m_machine_cnt) {
                for (int i = 1; i < m_machine_cnt; ++i) receiveMessage(i);
            }
            printf("received WM_END\n"); fflush(stdout);
        } else {
            printf("MW_NEXTSSSTART: %d\n", MW_NEXTSSSTART); fflush(stdout);
            sendAll(MW_NEXTSSSTART);
            printf("sent MW_NEXTSSSTART\n"); fflush(stdout);
        }
    }

    gettimeofday(&e_time, NULL);

    // 4. Set main thread to terminate.
    main_term = 1;
}

void Master::terminate() {
    printf("terminate\n"); fflush(stdout);

    // 1. Destroy Graph class.
    m_pmy_graph->term();
    GraphDestroyFn destroy_graph;
    destroy_graph = (GraphDestroyFn)dlsym(m_puf_handle, "destroy_graph");
    destroy_graph(m_pmy_graph);

    // 2. Free memory allocated.
    if (m_worker_msg) free(m_worker_msg);
    if (m_pfinish_send) free(m_pfinish_send);
    if (m_mw_nextssstart.aggr_global) free(m_mw_nextssstart.aggr_global);

    // 3. Quit receive & send threads.
    void* recv_retval;
    pthread_join(m_pth_receive, &recv_retval);
    void* send_retval;
    pthread_join(m_pth_send, &send_retval);

    // 4. Output elapsed.
    elapsed = (double)(e_time.tv_sec - b_time.tv_sec) + ((double)e_time.tv_usec - (double)b_time.tv_usec)/1e6;
    printf("elapsed time: %f seconds\n", elapsed); fflush(stdout);
}
