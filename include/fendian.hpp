#ifndef FENDIAN_H
#define FENDIAN_H

#include <cstdint>  // defines standard integer types like uint32_t
#include <cstring>  // for memcpy
#include <endian.h> // defines htobe32() and be32toh()

/*
    stdint.h is included for standard integer types like uint32_t.
    endian.h is included for the byte-swapping functions like htobe32 and be32toh.
    NOTE: The htobe32 and be32toh functions are cool because they don't have any effect if your host system is big endian already.
*/

typedef float float32_t; // just to be super duper clear that its 4 bytes IEEE-754 or whatever

// Union to represent a float as both a float and a uint32_t
typedef union {
    float32_t f;
    uint32_t u;
} float_union;

// Function to convert a float from host byte order to big-endian (network)
float f_htobe32(float32_t val) {
    float_union fu;
    memcpy(&fu.f, &val, sizeof(float32_t));
    fu.u = htobe32(fu.u); // Convert uint32_t to big-endian
    return fu.f;
}

// Function to convert a float from big-endian (network) to host byte order
float f_be32toh(float32_t val) {
    float_union fu;
    memcpy(&fu.f, &val, sizeof(float32_t));
    fu.u = be32toh(fu.u); // Convert uint32_t to host byte order
    return fu.f;
}

#endif /* FENDIAN_H */