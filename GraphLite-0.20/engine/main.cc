/**
 * @file main.cc
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
 * The main entrance for master and workers.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Master.h"
#include "Worker.h"

Master master;
Worker worker;

int main_term = 0; // main thread terminate flag, just for sender/receiver to
                   // test if break loop, not need mutex

void usage(char *cmd)
{
    printf("Master Usage: %s <machine id> <start script path> <user file path> [algorithm args]\n", cmd);
    printf("Worker Usage: %s <machine id> <user file path> [algorithm args]\n", cmd);
    exit(1);
}

int main(int argc, char* argv[]) {

    if (argc < 3) usage(argv[0]);

    int machine_id= atoi(argv[1]);

    if (strcmp(argv[1], "0") == 0) {

        printf("master run\n");
        if (argc < 4) usage(argv[0]);
        master.run(argc, argv);

    } else if ( machine_id > 0) {

        printf("worker run\n");
        worker.run(argc, argv);

    } else {
        usage(argv[0]);
    }

    return 0;
}
