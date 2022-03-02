#define DECLARE_FILE(name, prefix) extern "C" uint8_t _binary_##name##_start[]; extern "C" uint8_t _binary_##name##_end[]; struct prefix##name { static size_t getLength() { return _binary_##name##_end - _binary_##name##_start; } static uint8_t* getData() { return _binary_##name##_start; } };
DECLARE_FILE(close_png,)
DECLARE_FILE(closeActive_png,)
DECLARE_FILE(lock_png,)
DECLARE_FILE(lockActive_png,)
DECLARE_FILE(unlock_png,)
DECLARE_FILE(unlockActive_png,)
