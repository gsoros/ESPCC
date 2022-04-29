// #ifdef FEATURE_SERIAL

// #include "board.h"
// #include "split_stream.h"

// #if !defined(NO_GLOBAL_SPLITSTREAM) && !defined(NO_GLOBAL_INSTANCES)
// #ifndef SERIAL_INSTANCE
// #define SERIAL_INSTANCE
// #endif
// SplitStream Serial;
// #endif

// size_t SplitStream::write(const uint8_t *buffer, size_t size) {
//     board.api.write(buffer, size);
//     return Atoll::SplitStream::write(buffer, size);
// }

// #endif