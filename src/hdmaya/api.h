#ifndef __HDMAYA_API_H__
#define __HDMAYA_API_H__

// TODO: add flags for windows too! The current ones only for gcc and clang.
#if defined(HDMAYA_EXPORT)
#define HDMAYA_API __attribute__((visibility("default")))
#else
#define HDMAYA_API
#endif

#endif // __HDMAYA_API_H__
