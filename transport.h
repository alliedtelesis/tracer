#ifndef TRACE_TRANSPORT_H__
#define TRACE_TRANSPORT_H__

#define TRACE_TRANSPORT_ERROR_SOCK -1
#define TRACE_TRANSPORT_ERROR_CONN -2
#define TRACE_TRANSPORT_ERROR_ADDR -3

extern int trace_transport_close(int);

extern int trace_transport_unix(void);
extern int trace_transport_inet(void);

#endif /* TRACE_TRANSPORT_H__ */

