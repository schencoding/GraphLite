/**
 * @file Addr.h
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
 * This file defines struct Addr for storing machine information.
 *
 */

#ifndef _ADDR_H
#define _ADDR_H

#define NAME_LEN 128       // machine hostname length

/** Machine address structure. */
typedef struct Addr {
    int id;                  /**< machine id: master 0, worker from 1 */
    char hostname[NAME_LEN]; /**< machine hostname */
    int port;                /**< machine port for process */
} Addr;

#endif
