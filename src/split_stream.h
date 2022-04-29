// #if !defined(__split_stream_h) && defined(FEATURE_SERIAL)
// #define __split_stream_h

// #include "atoll_split_stream.h"

// class SplitStream : public Atoll::SplitStream {
//    public:
//     using Atoll::SplitStream::write;
//     virtual size_t write(const uint8_t *buffer, size_t size) override;
// };

// #if !defined(NO_GLOBAL_SPLITSTREAM) && !defined(NO_GLOBAL_INSTANCES)
// #ifndef SERIAL_INSTANCE
// #define SERIAL_INSTANCE
// #endif
// extern SplitStream Serial;
// #endif

// #endif