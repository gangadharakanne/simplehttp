#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "queue.h"
#include "simplehttp.h"
#include "request.h"
#include "stat.h"

extern int verbose;

struct simplehttp_request *simplehttp_request_new(struct evhttp_request *req, uint64_t id)
{
    struct simplehttp_request *s_req;
    simplehttp_ts start_ts;
    
    simplehttp_ts_get(&start_ts);
    s_req = malloc(sizeof(struct simplehttp_request));
    s_req->req = req;
    s_req->start_ts = start_ts;
    s_req->id = id;
    s_req->async = 0;
    s_req->index = -1;
    TAILQ_INSERT_TAIL(&simplehttp_reqs, s_req, entries);
    
    return s_req;
}

struct simplehttp_request *simplehttp_request_get(struct evhttp_request *req)
{
    struct simplehttp_request *entry;
    
    TAILQ_FOREACH(entry, &simplehttp_reqs, entries) {
        if (req == entry->req) {
            return entry;
        }
    }
    
    return NULL;
}

uint64_t simplehttp_request_id(struct evhttp_request *req)
{
    struct simplehttp_request *entry;
     
    entry = simplehttp_request_get(req);
    
    return entry ? entry->id : 0;
}

struct simplehttp_request *simplehttp_async_check(struct evhttp_request *req)
{
    struct simplehttp_request *entry;
    
    entry = simplehttp_request_get(req);
    if (entry && entry->async) {
        return entry;
    }
    
    return NULL;
}

void simplehttp_async_enable(struct evhttp_request *req)
{
    struct simplehttp_request *entry;
    
    if ((entry = simplehttp_request_get(req)) != NULL) {
        entry->async = 1;
    }
}

void simplehttp_request_finish(struct evhttp_request *req, struct simplehttp_request *s_req)
{
    simplehttp_ts end_ts;
    uint64_t req_time;
    char id_buf[64];
    
    simplehttp_ts_get(&end_ts);
    req_time = simplehttp_ts_diff(s_req->start_ts, end_ts);
    
    if (s_req->index != -1) {
        simplehttp_stats_store(s_req->index, req_time);
    }
    
    if (verbose) {
        sprintf(id_buf, "%"PRIu64, s_req->id);
        simplehttp_log("", req, req_time, id_buf);
    }
    
    TAILQ_REMOVE(&simplehttp_reqs, s_req, entries);
    free(s_req);
}

void simplehttp_async_finish(struct evhttp_request *req)
{
    struct simplehttp_request *entry;
    
    if ((entry = simplehttp_async_check(req))) {
        simplehttp_request_finish(req, entry);
    }
}
