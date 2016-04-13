/**
 * @file OutputFormatter.h
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
 * This file defined OutputFormatter class to provide output interface for user,
 * mainly to write local subgraph computation result.
 *
 * We provide an abstract method for users to override, and also some helpful
 * interfaces implemented by system.
 *
 */

#ifndef OUTPUTFORMATTER_H
#define OUTPUTFORMATTER_H

#include <fstream>
#include <stdint.h>

#include "hdfs.h"

/** Definition of OutputFormatter class. */
class OutputFormatter {
public:
    /** Definition of ResultIterator class. */
    class ResultIterator {
    public:
        /**
         * Get the result after computation.
         * @param vid reference of vertex id to be got
         * @param pvalue pointer of vertex value
         */
        void getIdValue(int64_t& vid, void* pvalue);

        /** Go to visit next element. */
        void next();

        /**
         * Judge if iterator terminates or not.
         * @retval true done
         * @retval false not
         */
        bool done();
    };

public:
    hdfsFile m_hdfs_file;       /**< output file handle, for hdfs */
    std::ofstream m_local_file; /**< output file stream, for local file system */

public:
    /**
     * Open output file, virtual method.
     * @param pout_path output file path
     */
    virtual void open(const char* pout_path);

    /** Close output file, virtual method. */
    virtual void close();

    /**
     * Write next result line, for user.
     * Write to current file offset.
     * @param pbuffer buffer of result line in string
     * @param len length of result line string
     */
    void writeNextResLine(char* pbuffer, int len);

    /** Write local subgraph computation result, pure virtual method. */
    virtual void writeResult() = 0;

    /** Destructor, virtual method. */
    virtual ~OutputFormatter() {}
}; // definition of OutputFormatter class

#endif /* OUTPUTFORMATTER_H */
