/**
 * @file Aggregator.h
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
 * This file defined Aggregator class to aggregate values among workers during
 * supersteps. Template argument is aggregated value type.
 *
 * Each worker accumulates values from vertices on it, and sends the local
 * result to master. Master merges values from all workers to a global value
 * and controls the process of supersteps through that to some degree.
 *
 * Different value types or methods correspond to different kinds of aggregator.
 * Master and workers may keep a series of different aggregators.
 *
 * @see AggregatorBase class
 *
 */

#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include "AggregatorBase.h"

/** Definition of Aggregator class. */
template<typename AggrValue>
class Aggregator: public AggregatorBase {
public:
    AggrValue m_global; /**< aggregator global value of AggrValue type */
    AggrValue m_local;  /**< aggregator local value of AggrValue type */

public:
    virtual void init() = 0;
    virtual int getSize() const {
        return sizeof(AggrValue);
    }
    virtual void* getGlobal() = 0;
    virtual void setGlobal(const void* p) = 0;
    virtual void* getLocal() = 0;
    virtual void merge(const void* p) = 0;
    virtual void accumulate(const void* p) = 0;
}; // definition of Aggregator class

#endif /* AGGREGATOR_H */
