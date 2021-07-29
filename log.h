
#ifndef PS2DRAW_LOG_H
#define PS2DRAW_LOG_H

#define log(level, msg, ...)\
  printf("[" level "] (%s : %d) " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_TRACE
#define LOG_DBG

#ifdef LOG_TRACE
#define trace(msg, ...) log("TRCE", msg, ##__VA_ARGS__)
#else
#define trace(msg, ...) ((void)0)
#endif

#ifdef LOG_DBG
#define log_dbg(msg, ...) log("DBG ", msg, ##__VA_ARGS__)
#else
#define log_dbg(msg, ...) ((void)0)
#endif 

#define info(msg, ...) log("INFO", msg, ##__VA_ARGS__)
#define warn(msg, ...) log("WARN", msg, ##__VA_ARGS__)
#define error(msg, ...) log("ERR ", msg, ##__VA_ARGS__)

#endif
